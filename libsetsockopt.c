#define _GNU_SOURCE

#include <dlfcn.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/*If SOL_TCP is already defined , priority must be given to SOL_TCP , otherwise go for IPPROTO_TCP*/
#if defined(__FreeBSD__) || defined(__OpenBSD__)
#ifndef SOL_TCP
#define SOL_TCP	IPPROTO_TCP
#endif
#endif

 
#ifdef __APPLE__
#ifndef SOL_TCP
#define SOL_TCP	IPPROTO_TCP
#endif

/*Apple/darwin derivative will use TCP_KEEPALIVE*/
#ifndef TCP_KEEPIDLE
#define TCP_KEEPIDLE TCP_KEEPALIVE
#endif
#endif

typedef int(*socket_func_t)(int domain, int type, int protocol);
typedef int(*setsockopt_func_t)(int sockfd, int level, int optname,
                                 const void *optval, socklen_t optlen);

void set_tcp_keepalive_params(int fd);

static socket_func_t socket_org = NULL;
static setsockopt_func_t setsockopt_org = NULL;

static int tcp_keepalive_state = -1;
static int tcp_keepalive_idle = -1;
static int tcp_keepalive_intvl = -1;
static int tcp_keepalive_cnt = -1;

void __attribute__ ((constructor)) setsockopt_init(void)
{
    socket_org = (socket_func_t) dlsym(RTLD_NEXT, "socket");
    if (!socket_org)
    {
        _exit(1);
    }

    setsockopt_org = (setsockopt_func_t) dlsym(RTLD_NEXT, "setsockopt");
    if (!setsockopt_org)
    {
        _exit(1);
    }

    // For now only tcp keepalive
    char *param = getenv("SETSOCKOPT_TCP_KEEPALIVE");
    if (param)
    {
        tcp_keepalive_state = atoi(param);

        if (tcp_keepalive_state >= 0)
        {
            param = getenv("SETSOCKOPT_TCP_KEEPALIVE_IDLE");
            if (param)
            {
                tcp_keepalive_idle = atoi(param);
            }

            param = getenv("SETSOCKOPT_TCP_KEEPALIVE_INTVL");
            if (param)
            {
                tcp_keepalive_intvl = atoi(param);
            }

            param = getenv("SETSOCKOPT_TCP_KEEPALIVE_CNT");
            if (param)
            {
                tcp_keepalive_cnt = atoi(param);
            }
        }
    }
}


int socket(int domain, int type, int protocol)
{
    if (!socket_org)
    {
        setsockopt_init();
    }

    int fd = socket_org(domain, type, protocol);

    if (fd > 0 &&
        tcp_keepalive_state >= 0 &&
        type == SOCK_STREAM &&
        domain == PF_INET)
    {
        set_tcp_keepalive_params(fd);
    }

    return fd;
}

int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen)
{
    if (!setsockopt_org)
    {
        setsockopt_init();
    }

    // Should we override again?
    return setsockopt_org(sockfd, level, optname, optval, optlen);
}

void set_tcp_keepalive_params(int fd)
{
    int errno_org = errno;

    setsockopt_org(fd, SOL_SOCKET, SO_KEEPALIVE,
                   &tcp_keepalive_state, sizeof(tcp_keepalive_state));

    if (tcp_keepalive_cnt >= 0)
    {
        setsockopt_org(fd, SOL_TCP, TCP_KEEPCNT,
                       &tcp_keepalive_cnt, sizeof(tcp_keepalive_cnt));
    }

    if (tcp_keepalive_idle)
    {
        setsockopt_org(fd, SOL_TCP, TCP_KEEPIDLE,
                       &tcp_keepalive_idle, sizeof(tcp_keepalive_idle));
    }

    if (tcp_keepalive_intvl)
    {
        setsockopt_org(fd, SOL_TCP, TCP_KEEPINTVL,
                       &tcp_keepalive_intvl, sizeof(tcp_keepalive_intvl));
    }

    errno = errno_org;
}
