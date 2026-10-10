#include "stdio_impl.h"
#include <sys/uio.h>

/* Dunit port: the kernel has no readv, so read straight into the caller's
 * buffer with the native Read (SYS_read). Upstream also tries to top up the
 * FILE's own buffer (iov[1]) in the same syscall as a readahead optimization;
 * we skip that (correct, just one fewer byte of buffering) rather than fake a
 * vector read. */
size_t __stdio_read(FILE *f, unsigned char *buf, size_t len)
{
	ssize_t cnt = syscall(SYS_read, f->fd, buf, len);
	if (cnt <= 0) {
		f->flags |= cnt ? F_ERR : F_EOF;
		return 0;
	}
	return (size_t)cnt;
}
