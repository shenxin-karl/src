#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <time.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "netword.h"

#define EPOLL_MAX 1024
#define BUFLEN 4096
#define HASH_MAX 24
#define HASH(fd) ((fd) % (HASH_MAX))
#define SERVER_PORT "8000"

typedef int (event_func)(int fd, int event, void *arg);
/* 自定义事件结构 */
typedef struct myevent_t {
        int                  e_fd;                  /* 文件描述符 */
        int                  e_event;               /* 监听的事件 */
        void                *e_arg;                 /* 事件函数参数 */
        event_func          *e_callback;            /* 事件对应的函数 */
        char                 e_buf[BUFLEN];         /* 缓冲区 */
        size_t               e_buflen;              /* 缓冲区字节数 */
        time_t               e_last_active;         /* 最后一次通信时间 */
        struct myevent_t    *e_next;                /* 指向下一结点 */
        struct myevent_t    *e_prev;                /* 指向上一结点 */
} myevent_t;


/* 哈希表 */
typedef struct hashtable_t {
    myevent_t    *h_buf[HASH_MAX];                  /* 哈希数组 */
    myevent_t    *h_listen;                         /* 监听数组 */
    size_t        h_size;                           /* 哈希表中的对象数量 */
} hashtable_t;


static int g_epfd;                                  /* epoll 红黑树根结点 */
static hashtable_t g_event_table;                   /* 哈希表 */

int initlistensock(int epfd, hashtable_t *table, char const *port);
int clean_timeout_connection(int epfd, hashtable_t *table);
int execute(int epfd, hashtable_t *table);
int destroy(int epfd, hashtable_t *table);
int event_add(int epfd, myevent_t *eventnode);
int event_del(int epfd, myevent_t *eventnode);
int event_mod(int epfd, myevent_t *eventnode);
int hashtable_add(hashtable_t *table, myevent_t *eventnode);
int hashtable_del(hashtable_t *table, myevent_t *eventnode);
int accept_connect(int fd, int event, void *arg);
int recvdata(int fd, int event, void *arg);
int sendtodata(int fd, int event, void *arg);
int process_data(char *buf, size_t len);

int main(int argc, char *argv[])
{
    char const *port = SERVER_PORT;

    if (argc == 2)
        port = argv[1];

    if ((g_epfd = epoll_create(EPOLL_MAX)) < 0) {
        fprintf(stderr, "%s: epoll_create error: %s\n", __func__, strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (initlistensock(g_epfd, &g_event_table, port) < 0) {
        fprintf(stderr, "initlistensock(%s) error\n", port);
        exit(EXIT_FAILURE);
    }

    printf("等待客户端连接\n");
    bzero(&g_event_table.h_buf, sizeof(g_event_table.h_buf));

    int ret;
    while (1) {
        clean_timeout_connection(g_epfd, &g_event_table);
        ret = execute(g_epfd, &g_event_table);

        if (ret < 0) {
            fprintf(stderr, "%s: execute error\n", __func__);
            break;
        }
    }

    destroy(g_epfd, &g_event_table);
    return 0;
}

/*
 * 函数说明:  初始化监听套接字, 将监听套接字加入到 epoll 红黑树中, 加入 hashtable 中
 * @epfd:     红黑树句柄
 * @table:    哈希表指针
 * @port:     绑定端口字符串
 */
int initlistensock(int epfd, hashtable_t *table, char const *port)
{
    if (port == NULL || epfd < 0 || table == NULL)
        return -1;

    int listenfd;
    if ((listenfd = tcp_server(NULL, port)) < 0)
        return -1;

    int flags = fcntl(listenfd, F_GETFD);
    flags |= O_NONBLOCK;
    fcntl(listenfd, F_SETFD, flags);

    myevent_t *eventnode;
    if ((eventnode = (myevent_t *)malloc(sizeof(myevent_t))) == NULL)
        return -1;

    bzero(eventnode, sizeof(myevent_t));
    eventnode->e_fd = listenfd;
    eventnode->e_event = EPOLLIN | EPOLLET;
    eventnode->e_callback = accept_connect;

    event_add(epfd, eventnode);
    table->h_listen = eventnode;

    return 0;
}



/*
 * 函数说明:    清除哈希表中已连接的套接字中, 超过 60s 没有通信的 套接字
 * @epfd:       红黑树句柄
 * @table:      指向哈希表
 */
int clean_timeout_connection(int epfd, hashtable_t *table)
{
    if (epfd < 0 || table == NULL)
        return -1;

    static int index = 0;
    time_t now = time(NULL);
    
    myevent_t *pnode = table->h_buf[index];
    myevent_t *delnode;
    while (pnode != NULL) {
        delnode = pnode;
        pnode = pnode->e_next;
        if ((delnode->e_last_active - now) > 60) {
            hashtable_del(table, delnode);
            event_del(epfd, delnode);
            close(delnode->e_fd);
            free(delnode);
        }
    }

    index = (index + 1) % HASH_MAX;
    return 0;
}

/*
 * 函数说明:    执行 epoll_wait, 并执行满足的套接字对应的 callback 回调函数
 * @epfd:       红黑树句柄
 * @table:      哈希表地址(暂时不用)
 */
int execute(int epfd, hashtable_t *table)
{
    if (epfd < 0 || table == NULL)
        return -1;

    struct epoll_event events[EPOLL_MAX];
    int readyn;
    
    if ((readyn = epoll_wait(epfd, events, EPOLL_MAX, -1)) < 0)
        return -1;
    
    for (int i = 0; i < readyn; ++i) {
        myevent_t *eventnode = (myevent_t *)events[i].data.ptr;
        eventnode->e_callback(eventnode->e_fd, eventnode->e_event, eventnode->e_arg);
    }

    return 0;
}


/*
 * 函数说明:    销毁函数, 释放 g_event_table 中的资源, 释放 g_epfd 
 * @epfd:       红黑树句柄
 * @table:      指向 g_event_table 的指针
 */
int destroy(int epfd, hashtable_t *table)
{
    if (epfd < 0 || table == NULL)
        return -1;

    myevent_t *pnode;
    myevent_t *delnode;
    for (int i = 0; i < HASH_MAX; ++i) {
        pnode = table->h_buf[i];
        while (pnode != NULL) {
            delnode = pnode;
            pnode = pnode->e_next;
            hashtable_del(table, delnode);
            close(delnode->e_fd);
            free(delnode);
        }
    }

    close(table->h_listen->e_fd);
    free(table->h_listen);

    return 0;
}


/*
 * 函数说明:    添加事件到 epfd 中
 * @epfd:       g_epfd 红黑树句柄
 * @eventnode:  事件节点指针
 */
int event_add(int epfd, myevent_t *eventnode)
{
    if (epfd < 0 || eventnode == NULL)
        return -1;

    struct epoll_event tep;

    bzero(&tep, sizeof(tep));
    tep.events = eventnode->e_event;
    tep.data.ptr = (void *)eventnode;

    return epoll_ctl(epfd, EPOLL_CTL_ADD, eventnode->e_fd, &tep);
}

/*
 * 函数说明:    删除 epfd 中的 eventnode->e_fd 节点
 * @epfd        红黑树句柄 g_epfd
 * @eventnode:  需要删除的节点
 */
int event_del(int epfd, myevent_t *eventnode)
{
    if (epfd < 0 || eventnode == NULL)
        return -1;

    return epoll_ctl(epfd, EPOLL_CTL_DEL, eventnode->e_fd, NULL);
}


/*
 * 函数说明:    修改 epfd 中的 eventnode->e_fd 的事件
 * @epfd:       红黑树句柄 g_epfd
 * @eventnode:  结点指针
 */
int event_mod(int epfd, myevent_t *eventnode)
{
    if (epfd < 0 || eventnode == NULL)
        return -1;

    struct epoll_event tep;

    bzero(&tep, sizeof(tep));
    tep.data.ptr = (void *)eventnode;

    if (eventnode->e_event & EPOLLIN) {
        eventnode->e_event = EPOLLOUT | EPOLLET;
        eventnode->e_callback = sendtodata;

    } else if (eventnode->e_event & EPOLLOUT) {
        eventnode->e_event = EPOLLIN | EPOLLET;
        eventnode->e_callback = recvdata;
    }

    eventnode->e_last_active = time(NULL);
    tep.events = eventnode->e_event;
    return epoll_ctl(epfd, EPOLL_CTL_MOD, eventnode->e_fd, &tep);
}


/*
 * 函数说明:    讲 myevent_t 节点加入到 hashtable_t 中
 * @table:      指向 g_event_table 的指针
 * @eventnode:  需要添加到 g_event_table 中的节点
 */
int hashtable_add(hashtable_t *table, myevent_t *eventnode)
{
    if (table == NULL || eventnode == NULL)
        return -1;
    int index;
    index = HASH(eventnode->e_fd);

    if (table->h_buf[index] != NULL) 
        table->h_buf[index]->e_prev = eventnode;

    eventnode->e_prev = NULL;
    eventnode->e_next = table->h_buf[index];
    table->h_buf[index] = eventnode;
    table->h_size++;

    return 0;
}

/*
 * 函数说明:    删除 g_event_table 中的节点
 * @table:      指向 g_event_table 的指针
 * @eventnode:  节点指针
 */
int hashtable_del(hashtable_t *table, myevent_t *eventnode)
{
    if (table == NULL || eventnode == NULL)
        return -1;

    if (eventnode->e_next != NULL)
        eventnode->e_next->e_prev = eventnode->e_prev;

    if (eventnode->e_prev != NULL)
        eventnode->e_prev->e_next = eventnode->e_next;
    
    int index;
    index = HASH(eventnode->e_fd);
    if (table->h_buf[index] == eventnode)
        table->h_buf[index] = eventnode->e_next;

    table->h_size--;
    
    return 0;
}


/*
 * 函数说明:    listenfd 回调函数, 当有客户发起连接时, 接受连接, 并创建 myevent_t 节点, 加入 g_event_table 和 g_epfd 中
 * @listenfd:   监听套接字
 * @event:      listenfd 的事件
 * @arg:        废弃不用(为了统一接口)
 */
int accept_connect(int listenfd, int event, void *arg)
{
    if (listenfd < 0 || !(event & EPOLLIN))
        return -1;

    myevent_t *eventnode;
    if ((eventnode = (myevent_t *)malloc(sizeof(myevent_t))) == NULL) {
        fprintf(stderr, "%s: malloc(sizeof(myevent_t )) error: %s\n", __func__, strerror(errno));
        return -1;
    }

    struct sockaddr_in addr;
    socklen_t addrlen;

    if ((eventnode->e_fd = accept(listenfd, (struct sockaddr *)&addr, &addrlen)) < 0) {
        free(eventnode);
        fprintf(stderr, "%s : accept error: %s\n", __func__, strerror(errno));
        return -1;
    }

    int flags = fcntl(eventnode->e_fd, F_GETFD);
    flags |= O_NONBLOCK;
    fcntl(eventnode->e_fd, F_SETFD, flags);

    char host[128];
    uint16_t port;

    inet_ntop(AF_INET, &addr.sin_addr.s_addr, host, sizeof(host));
    port = ntohs(addr.sin_port);
    printf("connection from %s:%d\n", host, port);

    eventnode->e_arg = (void *)eventnode;
    eventnode->e_next = NULL;
    eventnode->e_prev = NULL;
    eventnode->e_last_active = time(NULL);
    eventnode->e_event = EPOLLIN | EPOLLET;
    eventnode->e_callback = recvdata;

    event_add(g_epfd, eventnode);
    hashtable_add(&g_event_table, eventnode);

    return 0;
}


/*
 * 函数说明:    读事件回调函数
 * @fd:         与客户端连接的套接字
 * @event:      事件
 * @arg:        指向当前套接字 myevent_t * 的结点指针
 */
int recvdata(int fd, int event, void *arg)
{
    if (fd < 0 || arg == NULL || !(event & EPOLLIN))
        return -1;

    myevent_t *eventnode = (myevent_t *)arg;
    if ((eventnode->e_buflen = read(eventnode->e_fd, eventnode->e_buf, BUFLEN)) < 0)
        return -1;

    event_mod(g_epfd, eventnode);

    return 0;
}

/*
 * 函数说明:    写事件回调函数
 * @fd:         与客户端连接的套接字
 * @event:      事件
 * @arg:        指向 myevent_t * 结点的指针
 */
int sendtodata(int fd, int event, void *arg)
{
    if (fd < 0 || !(event & EPOLLOUT) || arg == NULL)
        return -1;

    myevent_t *eventnode = (myevent_t *)arg;
    process_data(eventnode->e_buf, eventnode->e_buflen);
    
    int writen;
    writen = write(fd, eventnode->e_buf, eventnode->e_buflen);

    event_mod(g_epfd, eventnode);
    return (writen < 0 ? -1 : 0);
}


/*
 * 函数说明:    数据处理函数, 小写转大写
 * @buf:        数据缓冲区指针
 * @len:        缓冲区长度
 */
int process_data(char *buf, size_t len)
{
    if (buf == NULL)
        return -1;

    for (size_t i = 0; i < len; ++i)
        buf[i] = toupper(buf[i]);

    return 0;
}
