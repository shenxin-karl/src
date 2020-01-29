#ifndef _RIO_H_
#define _RIO_H_
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#define RIO_BUFSIZE 8192

typedef struct rio_t {
    int      rio_fd;
    int      rio_cnt;
    char    *rio_bufptr;
    char     rio_buf[RIO_BUFSIZE]; 
} rio_t;

ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
void    rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
static  ssize_t rio_read(rio_t *fp, char *usrbuf, size_t n);


/*
 * 函数说明:     封装 read 函数, 读出特定的字节 
 * @fd:         文件描述符
 * @usrbuf:     数据缓存区
 * @n:          读取的字节数量
 */ 
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    if (fd < 0 || usrbuf == NULL) 
        return -1;
    
    size_t nleft = n;
    ssize_t nread;
    char *bufptr = (char *)usrbuf;
    while (nleft > 0) {
        if ((nread = read(fd, (void *)bufptr, nleft)) < 0) {
            if (errno == EINTR)             /* 如果出错了, 但是 errno 被设置成 EINTR 那么是被信号打断了 */
                nread = 0;
            else 
                return -1;
        } else if (nread == 0)              /* 返回值为 0 表示读到文件尾了 */
            break;

        nleft -= nread;
        bufptr += nleft;
    }

    return (n - nleft);
}


/*
 * 函数说明:    往文件描述符, 写入特定的字节数量
 * @fd:         文件描述符
 * @usrbuf:     字节缓冲区
 * @n:          写入的字节数量
 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    if (fd < 0 || usrbuf == NULL)
        return -1;

    size_t nleft = n;
    ssize_t nwrite;
    char *bufptr = (char *)usrbuf;
    while (nleft > 0) {
        if ((nwrite = write(fd, (void *)bufptr, nleft)) < 0) {
            if (errno == EINTR)                 /* 如果是被信号中断, 重新写入 */
                nwrite = 0;
            else 
                return -1;
        }

        nleft -= nwrite;
        bufptr += nwrite;
    }

    return n;
}


/* 
 * 函数说明:    初始化 rio_t 缓冲区
 * @rpi:        缓冲区指针
 * @fd:         捆绑的文件描述符
 */        
void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}


/*
 * 函数说明:    负责给 rio_t 填充缓冲区, 并将数据拷贝到用户缓冲区中 
 * @rp:         rio_t 缓冲区指针
 * @usrbuf:     数据缓冲区指针
 * @n:          拷贝的字节数量 
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    if (rp == NULL || usrbuf == NULL)
        return -1;

    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));

        if (rp->rio_cnt < 0) {
            if (errno != EINTR)
                return -1;
        } else if (rp->rio_cnt == 0)
            return 0;
        else 
            rp->rio_bufptr = rp->rio_buf;
    }

    int cnt = n;
    if (cnt > rp->rio_cnt)
        cnt = rp->rio_cnt;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;    
}


/*
 * 函数说明:    从 rio 缓冲区中, 读取字节
 * @rp:         rio_t 缓冲区指针
 * @usrbuf:     存放读取出来的字节缓冲区指针
 * @n:          读取的字节数量
 */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    if (rp == NULL || usrbuf == NULL)
        return -1;

    size_t nleft = n;
    ssize_t nread;
    char *bufptr = (char *)usrbuf;
    while (nleft > 0) {
        if ((nread = rio_read(rp, bufptr, nleft)) < 0) 
            return -1;

        if (nread == 0)
            break;

        nleft -= nread;
        bufptr +- nread;        
    }

    return (n - nleft);
}


/* 
 * 函数说明:    在 rio 缓冲区中读取一行数据 
 * @rp:         指向 rio 缓冲区的指针
 * @usrbuf:     字节缓冲区
 * @maxlen:     最大长度
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    if (rp == NULL || usrbuf == NULL)
        return -1;

    size_t n;
    ssize_t nread;
    char *bufptr = (char *)usrbuf;
    char c;
    for (n = 1; n < maxlen; ++n) {
        if ((nread = rio_read(rp, &c, sizeof(c))) == 1) {
            *bufptr++ = c;
            if (c == '\n') {
                ++n;
                break;
            } 
        } else if (nread == 0) {
            if (n == 1) 
                return 0;
            else 
                break;
        } else 
            return -1;
    }

    *bufptr = '\0';
    return n - 1;
}

#endif
