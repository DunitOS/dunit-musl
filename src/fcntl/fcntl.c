#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include "syscall.h"

/* Dunit port: fcntl subset.
 *
 * F_DUPFD / F_DUPFD_CLOEXEC are built on the Dunit Dup syscall via dup() (see
 * dunit_dupfd); CLOEXEC is a documented no-op (no fd-preserving exec yet).
 *
 * F_GETFD/F_SETFD (FD_CLOEXEC) and F_GETFL/F_SETFL (status flags: access mode,
 * O_APPEND, O_NONBLOCK) go to the Dunit Fcntl syscall, which keeps per-fd flag
 * state; O_NONBLOCK is honoured on pipe reads. Locks/owner and other commands
 * return the kernel's documented -ENOSYS (M6.1), not a fake value. */
#define DUNIT_SYS_FCNTL 86

static int dunit_dupfd(int fd, int min)
{
	if (min < 0)
		return -EINVAL;
	int held[128];
	int n = 0;
	int result;
	for (;;) {
		int r = dup(fd);
		if (r < 0) { result = -errno; break; }
		if (r >= min) { result = r; break; }
		if (n >= (int)(sizeof held / sizeof held[0])) { close(r); result = -EMFILE; break; }
		held[n++] = r;
	}
	for (int i = 0; i < n; i++)
		close(held[i]);
	return result;
}

int fcntl(int fd, int cmd, ...)
{
	unsigned long arg;
	va_list ap;
	va_start(ap, cmd);
	arg = va_arg(ap, unsigned long);
	va_end(ap);

	switch (cmd) {
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
		return __syscall_ret(dunit_dupfd(fd, (int)arg));
	default:
		return __syscall_ret(__syscall(DUNIT_SYS_FCNTL, fd, cmd, arg));
	}
}
