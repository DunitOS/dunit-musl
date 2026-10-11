#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "syscall.h"

/* Dunit port: dup2(old, new) maps onto the Dunit Dup syscall with newfd = new.
 * The kernel handles the old==new case (returns new without closing). */
#define DUNIT_SYS_DUP 82

int dup2(int old, int new)
{
	return __syscall_ret(__syscall(DUNIT_SYS_DUP, old, new));
}
