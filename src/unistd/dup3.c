#define _GNU_SOURCE
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "syscall.h"

/* Dunit port: dup3(old, new, flags). POSIX requires EINVAL when old == new.
 * The only defined flag is O_CLOEXEC; Dunit has no exec yet, so close-on-exec
 * has no observable effect and the flag is accepted but not recorded (documented
 * — revisit when exec lands). Any other flag is rejected. */
#define DUNIT_SYS_DUP 82

int __dup3(int old, int new, int flags)
{
	if (old == new)
		return __syscall_ret(-EINVAL);
	if (flags & ~O_CLOEXEC)
		return __syscall_ret(-EINVAL);
	return __syscall_ret(__syscall(DUNIT_SYS_DUP, old, new));
}

weak_alias(__dup3, dup3);
