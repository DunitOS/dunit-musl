#include "stdio_impl.h"
#include <sys/uio.h>

/* Dunit port: the kernel has no writev, so drain the FILE's pending buffer and
 * then the new data using the native Write (SYS_write), one buffer at a time.
 * The partial-write / error accounting matches upstream __stdio_write: an error
 * before any of the new `buf` has gone out reports 0, otherwise it reports how
 * many bytes of `buf` were written. Non-cancellable, like upstream stdio. */
size_t __stdio_write(FILE *f, const unsigned char *buf, size_t len)
{
	const unsigned char *parts[2] = { f->wbase, buf };
	size_t sizes[2] = { (size_t)(f->wpos - f->wbase), len };

	for (int i = 0; i < 2; i++) {
		const unsigned char *p = parts[i];
		size_t left = sizes[i];
		while (left) {
			ssize_t cnt = syscall(SYS_write, f->fd, p, left);
			if (cnt <= 0) {
				f->wpos = f->wbase = f->wend = 0;
				f->flags |= F_ERR;
				return i == 0 ? 0 : len - left;
			}
			p += cnt;
			left -= (size_t)cnt;
		}
	}

	f->wend = f->buf + f->buf_size;
	f->wpos = f->wbase = f->buf;
	return len;
}
