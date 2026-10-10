#include <sys/uio.h>
#include <unistd.h>
#include "syscall.h"

/* Dunit port: no readv in the kernel. Fill the iovecs in order with the native
 * Read (SYS_read), stopping at the first short/EOF read so a streaming fd is not
 * blocked waiting to fill later buffers. Returns total bytes read, or -1 with
 * errno set if the first read fails. */
ssize_t readv(int fd, const struct iovec *iov, int count)
{
	ssize_t total = 0;
	for (int i = 0; i < count; i++) {
		if (iov[i].iov_len == 0)
			continue;
		ssize_t r = syscall_cp(SYS_read, fd, iov[i].iov_base, iov[i].iov_len);
		if (r < 0)
			return total ? total : r;
		total += r;
		if ((size_t)r < iov[i].iov_len)
			break;
	}
	return total;
}
