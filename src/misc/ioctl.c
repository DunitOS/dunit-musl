/* Translate the documented terminal subset to native bounded records.
 * Linux ioctl request numbers and termios layouts never cross the Dunit ABI. */
#include <sys/ioctl.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include "syscall.h"

hidden long __dunit_ioctl(int fd, int req, void *arg)
{
    long op, size;
    switch (req) {
    case TIOCGWINSZ: op = 1; size = 8; break;
    case TIOCSWINSZ: op = 2; size = 8; break;
    case FIONREAD: op = 3; size = 4; break;
    default: {
        long r = __syscall(88, fd, 0, 0, 0);
        return r < 0 ? r : -EOPNOTSUPP;
    }
    }
    return __syscall(88, fd, op, arg, size);
}

int ioctl(int fd, int req, ...)
{
    void *arg = 0;
    if (req == TIOCGWINSZ || req == TIOCSWINSZ || req == FIONREAD) {
        va_list ap; va_start(ap, req); arg = va_arg(ap, void *); va_end(ap);
    }
    return __syscall_ret(__dunit_ioctl(fd, req, arg));
}
