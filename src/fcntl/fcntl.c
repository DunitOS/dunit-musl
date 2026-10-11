#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include "syscall.h"

/* Dunit port: fcntl subset.
 *
 * F_DUPFD / F_DUPFD_CLOEXEC are real, built on the Dunit Dup syscall via dup():
 * dup() returns the lowest free fd, so to honour the `arg` floor we keep dup'ing
 * and close the fds that landed below it. CLOEXEC has no effect yet — Dunit has
 * no fd-preserving exec (posix_spawn file-actions are unimplemented), so the
 * flag has nothing to act on; the dup itself happens, which is why this is a
 * documented limitation, not a faked success.
 *
 * Everything else (F_GETFL/F_SETFL/F_GETFD/F_SETFD, locks, owner) needs per-fd
 * kernel flag state that does not exist yet, so it returns a documented -ENOSYS
 * (M6.1) until a Dunit Fcntl syscall lands. */

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
		return __syscall_ret(-ENOSYS);
	}
}
