#ifndef _NETWORD_HPP_
#define _NETWORD_HPP_
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * 函数说明: 使用 tcp 连接客户端, 成功返回 套接字文件描述符, 内部 getaddrinfo 失败返回错误号, 其余失败返回 -1
 * @host:    服务器地址
 * @port:    服务器端口
 */
int tcp_client(char const *host, char const *port)
{
    if (host == NULL || port == NULL)
        return -1;

    struct addrinfo hints;
    struct addrinfo *listp;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;

    int err;
    if ((err = getaddrinfo(host, port, &hints, &listp)) < 0)
        return err;

    struct addrinfo *p;
    int clientfd;
    for (p = listp; p != NULL; p = p->ai_next) {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        if (connect(clientfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(clientfd);
    }

    freeaddrinfo(listp);
    listp = NULL;

    return (p == NULL ? -1 : clientfd);
}


/*
 * 函数说明: 建立服务器 listen 监听套接字, 成功返回 套接字文件描述符, 失败返回 -1
 * @host:    绑定的 域名(可选)
 * @port:    绑定的端口号
 */
int tcp_server(char const *host, char const *port)
{
    if (port == NULL) 
        return -1;

    struct addrinfo hints;
    struct addrinfo *listp;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE | AI_NUMERICSERV;

    int err;
    if ((err = getaddrinfo(host, port, &hints, &listp)) < 0)
        return err;

    struct addrinfo *p;
    int listenfd;
    int opt = 1;
    for (p = listp; p != NULL; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(listenfd);
    }

    freeaddrinfo(listp);
    listp = NULL;

    if (p == NULL)
        return -1;

    if (listen(listenfd, 128) < 0)
        return -1;

    return listenfd;
}


/*
 * 函数说明:  传递 域名 和 端口 返回 udp 的客户端的 套接字地址结构, 成功返回 sockfd 文件描述符, 失败返回 -1
 * @host:     域名
 * @port:     端口号
 * @addr:     返回套接字地址结构
 * @addrlen:  套接字地址结构的长度
 */
int udp_client(char const *host, char const *port, struct sockaddr_in *addr, socklen_t *addrlen)
{
    if (host == NULL || port == NULL || addr == NULL || addrlen == NULL)
        return -1;

    struct addrinfo hints;
    struct addrinfo *listp;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;
    
    int err;
    if ((err = getaddrinfo(host, port, &hints, &listp)) < 0)
        return err;

    struct addrinfo *p;
    int sockfd;
    for (p = listp; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) >= 0)
            break;
    }

    if (p != NULL) {
        memcpy(addr, p->ai_addr, p->ai_addrlen);
        *addrlen = p->ai_addrlen;
    }

    freeaddrinfo(listp);
    listp = NULL;

    return (p == NULL ? -1 : sockfd);
}


/*
 * 函数说明:  使用 upd 协议, 进行连接, 成功是返回 connect 到 host:port 的服务器的 udp 套接字, 失败返回 -1
 * @host:     域名
 * @port:     端口号
 */
int udp_connect(char const *host, char const *port)
{
    if (host == NULL || port == NULL)
        return -1;

    struct addrinfo hints;
    struct addrinfo *listp;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AI_NUMERICSERV | AI_ADDRCONFIG;

    int err;
    if ((err = getaddrinfo(host, port, &hints, &listp)) < 0)
        return err;

    struct addrinfo *p;
    int connfd;
    for (p = listp; p != NULL; p = p->ai_next) {
        if ((connfd = socket(p->ai_family, p->ai_socktype, p->ai_addrlen)) < 0)
            continue;

        if (connect(connfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(connfd);
    }

    freeaddrinfo(listp);
    listp = NULL;

    return (p == NULL ? -1 : connfd);
}


/* 
 * 函数说明:  给定 host:port 返回 bind 成功的 sockfd, 失败返回 -1
 * @host:     域名(可选)
 * @port:     端口号
 */
int udp_server(char const *host, char const *port)
{
    if (port == NULL)
        return -1;

    struct addrinfo hints;
    struct addrinfo *listp;

    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_DCCP;
    hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE | AI_NUMERICSERV;

    int err;
    if ((err = getaddrinfo(host, port, &hints, &listp)) < 0)
        return err;
    
    struct addrinfo *p;
    int sockfd;
    for (p = listp; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(sockfd);
    }

    freeaddrinfo(listp);
    listp = NULL;

    return (p == NULL ? -1 : sockfd);
}

#endif
