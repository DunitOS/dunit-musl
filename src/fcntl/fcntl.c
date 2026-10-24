#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "syscall.h"
int fcntl(int fd, int cmd, ...) {
    long arg=0;
    if (cmd != F_GETFD && cmd != F_GETFL && cmd != F_GETOWN) {
        va_list ap; va_start(ap,cmd); arg=va_arg(ap,long); va_end(ap);
    }
    return __syscall_ret(__dunit_fcntl(fd,cmd,arg));
}
