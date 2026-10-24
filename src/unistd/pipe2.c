#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "syscall.h"

int pipe2(int fd[2], int flag)
{
	if (flag & ~(O_CLOEXEC|O_NONBLOCK)) { errno = EINVAL; return -1; }
    return __syscall_ret(__syscall(86, ((flag&O_CLOEXEC)?128:0)|((flag&O_NONBLOCK)?64:0), 6, fd));
}
