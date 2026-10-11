#include <unistd.h>
#include "syscall.h"

/* Dunit port: dup(fd) maps onto the Dunit Dup syscall with newfd = -1 (pick the
 * lowest free fd). Dup(oldfd, newfd): newfd < 0 = dup, newfd >= 0 = dup2. */
#define DUNIT_SYS_DUP 82

int dup(int fd)
{
	return __syscall_ret(__syscall(DUNIT_SYS_DUP, fd, -1));
}
