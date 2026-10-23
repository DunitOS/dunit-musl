#include <sys/uio.h>
#include <unistd.h>
#include "dunit_io.h"

/* Dunit port: the Green Tea Kernel exposes a native Write (one buffer) but no
 * writev, so emulate the vector write with a Write per iovec. Returns the total
 * number of bytes written, or -1 with errno set if the first write fails. */
ssize_t writev(int fd, const struct iovec *iov, int count)
{
	ssize_t total = 0;
	for (int i = 0; i < count; i++) {
		const unsigned char *p = iov[i].iov_base;
		size_t left = iov[i].iov_len;
		while (left) {
			ssize_t r = __syscall_ret(__dunit_write(fd, p, left, 1));
			if (r < 0) return total ? total : r;
			if (r == 0) return total;
			p += r;
			left -= (size_t)r;
			total += r;
		}
	}
	return total;
}
