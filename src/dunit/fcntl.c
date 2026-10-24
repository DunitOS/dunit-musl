#define _GNU_SOURCE
#include <fcntl.h>
#include <errno.h>
#include "syscall.h"

hidden long __dunit_fcntl(int fd, int cmd, long arg)
{
    long op;
    switch (cmd) {
    case F_GETFD: op=0; arg=0; break;
    case F_SETFD: if (arg & ~FD_CLOEXEC) return -EINVAL; op=1; arg=!!(arg & FD_CLOEXEC); break;
    case F_GETFL: {
        long r=__syscall(86,fd,2,0);
        if(r<0)return r;
        return ((r&3)==3?O_RDWR:(r&2)?O_WRONLY:O_RDONLY) | ((r&16)?O_APPEND:0) | ((r&64)?O_NONBLOCK:0);
    }
    case F_SETFL:
        if(arg & ~(O_ACCMODE|O_APPEND|O_NONBLOCK|O_LARGEFILE|O_NOCTTY))return -EOPNOTSUPP;
        op=3; arg=((arg&O_APPEND)?16:0)|((arg&O_NONBLOCK)?64:0); break;
    case F_DUPFD: op=4; break;
    case F_DUPFD_CLOEXEC: op=5; break;
    default: return -EOPNOTSUPP;
    }
    return __syscall(86,fd,op,arg);
}
