#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <setjmp.h>
#include "rio.h"
#include "network.h"

#define output_error_message(...)                           \
        fprintf(stderr, "%s:%d: ", __func__, __LINE__);     \
        fprintf(stderr, __VA_ARGS__);

#define MAXLINE  1024

extern char **environ;

void doit(int fd);
void read_requsthdrs(rio_t *rp);
int  parse_uri(char *uri, char *filename, char *cgiargs);
void server_static(int fd, char *filename, int filesize);
void server_dynamic(int fd, char *filename, char *cgiargs);
void get_filetype(char const *filename, char *filetype);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sig_chld(int signo);
void sig_pipe(int signo);
void sginal_captrue();

static sigjmp_buf env;
static volatile sig_atomic_t canjmp;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        output_error_message("error: %s + port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sginal_captrue();                       /* 注册信号捕获函数 */

    int listenfd;
    if ((listenfd = open_listenfd("127.0.0.1", argv[1])) < 0) {
        output_error_message("open_listenfd(NULL, %s) error\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    int connfd;
    socklen_t addrlen;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE];
    char port[MAXLINE];
    while (1) {
        printf("Waiting for connnect...\n");
        addrlen = sizeof(addrlen);
        if ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen)) < 0) {
            output_error_message("accpet error: %s", strerror(errno));
            break;
        }

        getnameinfo((struct sockaddr *)&clientaddr, addrlen, hostname, 
                    sizeof(hostname), port, sizeof(port), NI_NUMERICHOST); 

        printf("connection form %s:%s\n", hostname, port);
        if (sigsetjmp(env, 1) == 0) {
            canjmp = 1;
            doit(connfd);
        }
    
        close(connfd);
    }


    close(listenfd);
}


/* 
 * 函数说明:        执行客户端发生的 http 请求
 * @fd:             与客户端连接的套接字文件描述符
 */
void doit(int fd)
{
    rio_t rio;
    char buf[MAXLINE];
    char version[MAXLINE];
    char method[MAXLINE];
    char uri[MAXLINE];

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, sizeof(buf));
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    /* 如果不是 GET 方法, 那么出错返回 */
    if (strcasecmp(method, "GET")) {
        output_error_message("method is not \"GET\"\n");
        clienterror(fd, method, "501", "Not implemented", "Tiny does not inplement this method");
        return;
    }

    read_requsthdrs(&rio);                  /* 忽略其余报头信息 */

    /* 解析 uri 路径 */
    char filename[MAXLINE];
    char cgiargs[MAXLINE];
    int is_static;
    is_static = parse_uri(uri, filename, cgiargs);

    /* 获得文件属性 */
    struct stat sbuf;
    if (stat(filename, &sbuf) < 0) {
        output_error_message("filename: %s -- stat error:%s\n", filename, strerror(errno));
        clienterror(fd, filename, "404", "Not found", "Tiny colun't find this file");
        return;
    }

    /* 静态文件 */
    if (is_static) {
        if (!S_ISREG(sbuf.st_mode) || !(sbuf.st_mode & S_IRUSR)) {
            output_error_message("%s: not permisson read the file\n", filename);
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        server_static(fd, filename, sbuf.st_size);

    /* 动态文件 */
    } else {
        if (!S_ISREG(sbuf.st_mode) || !(sbuf.st_mode & S_IXUSR)) {
            output_error_message("%s: not permisson excute the file\n", filename);
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the file");
            return;
        }
        server_dynamic(fd, filename, cgiargs);
    }
}


/*
 * 函数功能:    忽略 http 请求中的报头
 * @rp:         指向绑定了 客户端文件描述符的 rio 缓冲区指针
 */
void read_requsthdrs(rio_t *rp)
{
    char buf[MAXLINE];
    int  len;

    do {
        len = rio_readlineb(rp, buf, sizeof(buf));
        if (len > 0) {
            buf[len] = '\0';
            printf("%s", buf);
        }
        
    } while (strcmp(buf, "\r\n") != 0 && len > 0);

    printf("\n");
}

/* 
 * 函数说明:    解析 uri, 获取文件名 和 参数
 * @uri:        uri 字符串
 * @filename:   文件名存放缓冲区
 * @cgiargs:    参数缓冲区
 */        
int parse_uri(char *uri, char *filename, char *cgiargs)
{
    char *p = NULL;

    /* 动态文件 */
    if (strstr(uri, "cgi-bin")) {
        p = index(uri, '?');

        if (p != NULL) {
            strcpy(cgiargs, p + 1);
            *p = '\0';
        } else 
            strcpy(cgiargs, "");

        strcpy(filename, ".");
        strcat(filename, uri);
        return 0;

    /*  静态文件 */
    } else {
        strcpy(filename, ".");
        strcpy(cgiargs, "");
        strcat(filename, uri);
        if (filename[strlen(filename) - 1] == '/') 
            strcat(filename, "home.html");
        
        return 1;
    }
}

/*
 * 函数说明:    给客户端发送静态文件
 * @fd:         与客户端连接的套接字文件描述符
 * @filename:   需要发送的文件名
 * @filesize:   文件大小
 */
void server_static(int fd, char *filename, int filesize)
{
    char buf[MAXLINE];
    char filetype[MAXLINE];

    /* 创建响应报头 */
    get_filetype(filename, filetype);
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-lenght: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n", buf, filetype);
    sprintf(buf, "%s\r\n", buf);

    /* 发送文件内容 */
    int srcfd;
    if ((srcfd = open(filename, O_RDONLY)) < 0) {
        output_error_message("open(%s) error: %s\n", filename, strerror(errno));
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't Unable to read file contents");
        return;
    }

    // void *srcp;
    // if ((srcp = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0)) == MAP_FAILED) {
    //     output_error_message("mmap error: %s\n", strerror(errno));
    //     clienterror(fd, filename, "403", "Forbidder", "Tiny couldn't Unable to read file contents");
    //     return;
    // }

    // close(srcfd);
    // rio_writen(fd, buf, strlen(buf));
    // rio_writen(fd, srcp, filesize);
    // munmap(srcp, filesize);

    /* 使用 malloc 实现发送数据 */
    void *srcp;
    if ((srcp = malloc(filesize)) == NULL) {
        output_error_message("malloc error: %s\n", strerror(errno));
        clienterror(fd, filename, "403", "Forbidder", "Tiny couldn't Unable to read file contents");
        return;
    }

    int read_size = rio_readn(srcfd, srcp, filesize);
    if (read_size != filesize) {
        output_error_message("rio_readnb error: %s\n", strerror(errno));
        clienterror(fd, filename, "403", "Forbidder", "Tiny coludn't Unable to read file contents");
        free(srcp);
        return;
    }

    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, srcp, filesize);
    free(srcp);
    close(srcfd);
}

/*
 * 函数功能:    执行 cgi 程序
 * @fd:         与客户端通信的文件描述符
 * @filename:   文件名
 * @cgiargs:    cgi 程序参数字符串
 */
void server_dynamic(int fd, char *filename, char *cgiargs)
{
    char buf[MAXLINE];
    char *emptylist[] = {NULL};

    /* 创建响应报头 */
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);

    /* 执行 cgi */
    pid_t pid;
    if ((pid = fork()) < 0) {
        output_error_message("fork error: %s\n", strerror(errno));
        clienterror(fd, filename, "403", "Forbidder", "Tiny Web Server cannot execute the program");
        return;
    
    } else if (pid == 0) {
        rio_writen(fd, buf, strlen(buf));           /* 发送响应报头 */
        setenv("QUERY_STRING", cgiargs, 1);       
        dup2(fd, STDOUT_FILENO);
        execve(filename, emptylist, environ);
    }

    return;
}
/*
 * 函数说明:    给定文件名, 获得文件类型在 http 中的表示
 * @filename:   文件名
 * @filetype:   存放文件类型的缓冲区 
 */
void get_filetype(char const *filename, char *filetype)
{
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");

    else if (strstr(filename, ".gif")) 
        strcpy(filetype, "image/gif");

    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");

    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");

    else if (strstr(filename, ".mpg"))
        strcpy(filetype, "video/mpg"); 
    
    else if (strstr(filename, ".mp3"))
        strcpy(filetype, "audio/mp3");

    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mpeg4");

    else 
        strcpy(filetype, "text/plain");
}


/* 
 * 函数功能:    给个客户端发送错误信息
 * @fd:         与客户端连接的套节字文件描述符
 * @cause:      错误来源 
 * @errnum:     错误号
 * @shortmsg:   短错误原因描述
 * @longmsg:    长错误原因描述
 */ 
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
    char buf[MAXLINE];
    char body[MAXLINE];

    /* 创建 html 响应文件 */
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body, "%s<body bgcolor=\"ffffff\">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web Server</em>\r\n", body);

    /* 创建 http 响应报头 */
    int body_len = strlen(body);
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    sprintf(buf, "%sContent-type: text/html\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, body_len);
    sprintf(buf, "%sConnection: close\r\n\r\n", buf);

    /* 发送 */
    rio_writen(fd, buf, strlen(buf));
    rio_writen(fd, body, body_len); 
}

/*
 * 函数说明:    注册 SIGCHLD 信号处理函数, 并在信号中回收子进程
 */ 
void sig_chld(int signo)
{
    while (waitpid(0, NULL, WNOHANG) > 0);      /* 非阻塞循环回收子进程 */
}


/*
 * 函数说明:    对 SIGCHLD SIGPIPE 信号进行捕获 
 */
void sginal_captrue()
{
    struct sigaction action;

    action.sa_handler = sig_chld;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &action, NULL) < 0) {
        output_error_message("sigaction error: %s \n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    action.sa_handler = sig_pipe;
    if (sigaction(SIGPIPE, &action, NULL) <0) {
        output_error_message("sigacion error: %s \n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}


/* 
 * 函数说明:    SIGPIPE 信号捕捉函数, 当 写入 connfd 对端关闭连接, 系统发送 SIGPIPE 信号, 
 *              使得进程可以完美处理异常, 通过调用 siglongjmp 回到 main 函数中
 */
void sig_pipe(int signo)
{
    if (!canjmp)
        return;

    canjmp = 0;
    printf("\n\nerror: The client has colsed the connection and data sent is lost\n\n");
    siglongjmp(env, 1);
}
