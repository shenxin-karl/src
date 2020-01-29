#ifndef _NETWORK_H_ 
#define _NETWORK_H_
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>


/* 
 * 函数说明:    创建 TCP 监听套接字文件描述符, 并设置端口复用
 * @host:       绑定的 主机名(可选)
 * @port:       绑定的端口号
 */
int open_listenfd(char const *host, char const *port)
{
    if (port == NULL)
        return -1;
    
    struct addrinfo hints;
    struct addrinfo *listp;
    
    bzero(&hints, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV | AI_PASSIVE;

    int err;
    if ((err = getaddrinfo(host, port, &hints, &listp)) < 0)
        return err;
    
    struct addrinfo *p;
    int listenfd;
    int sockopt = 1;
    for (p = listp; p != NULL; p = p->ai_next) {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;
        
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        
        close(listenfd);        
    } 

    freeaddrinfo(listp);
    listp = NULL;

    if (p == NULL)
     return -1;

    if (listen(listenfd, 128) < 0) {
        close(listenfd);
        return -1;
    }

    return listenfd;
}

#endif
