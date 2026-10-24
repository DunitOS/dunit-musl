#include <poll.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include "syscall.h"
int poll(struct pollfd *fds, nfds_t n, int timeout)
{
    if(n>1024){errno=EINVAL;return -1;}
    if(n && !fds){errno=EFAULT;return -1;}
    uint64_t deadline=0;
    if(timeout<0)deadline=UINT64_MAX;
    else if(timeout>0){
        struct timespec ts;
        if(clock_gettime(CLOCK_MONOTONIC,&ts))return -1;
        deadline=(uint64_t)ts.tv_sec*1000000000+ts.tv_nsec+(uint64_t)timeout*1000000;
    }
    struct native_poll {int32_t fd; uint16_t events,revents;} *records=calloc(n?n:1,sizeof *records);
    if(!records){errno=ENOMEM;return -1;}
    for(nfds_t i=0;i<n;i++){
        if(fds[i].events & ~(POLLIN|POLLOUT|POLLRDNORM|POLLWRNORM)){free(records);errno=EOPNOTSUPP;return -1;}
        records[i].fd=fds[i].fd;
        records[i].events=((fds[i].events&(POLLIN|POLLRDNORM))?1:0)|((fds[i].events&(POLLOUT|POLLWRNORM))?2:0);
    }
    long r=__syscall(87,records,n,deadline);
    if(r>=0)for(nfds_t i=0;i<n;i++){
        unsigned v=records[i].revents;
        fds[i].revents=((v&1)?(fds[i].events&(POLLIN|POLLRDNORM)):0)|((v&2)?(fds[i].events&(POLLOUT|POLLWRNORM)):0)|((v&4)?POLLHUP:0)|((v&8)?POLLERR:0)|((v&16)?POLLNVAL:0);
    }
    free(records);
    return __syscall_ret(r);
}
