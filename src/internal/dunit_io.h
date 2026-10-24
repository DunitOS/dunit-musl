#ifndef DUNIT_IO_H
#define DUNIT_IO_H
#include <errno.h>
#include <stddef.h>
#include <fcntl.h>
#include <poll.h>
#include "syscall.h"

/* Transfer granularity, not desktop policy or an ABI limit. Page-sized
 * operations fit the native bounded-copy contract. POSIX permits short reads
 * and writes; stdio and writev drain them without losing data. */
#define DUNIT_IO_CHUNK 4096
static inline long __dunit_read(int fd, void *buf, size_t count, int cancellable)
{
	if (count > DUNIT_IO_CHUNK) count = DUNIT_IO_CHUNK;
	for (;;) {
		long result = cancellable ? __syscall_cp(SYS_read, fd, buf, count)
			: __syscall(SYS_read, fd, buf, count);
        if (result != -EAGAIN) return result;
        long flags = __dunit_fcntl(fd, F_GETFL, 0);
        if (flags < 0 || (flags & O_NONBLOCK)) return result;
        struct pollfd input = {fd, POLLIN, 0};
        if (poll(&input, 1, -1) < 0) return -errno;
	}
}
static inline long __dunit_write(int fd, const void *buf, size_t count, int cancellable)
{
	if (count > DUNIT_IO_CHUNK) count = DUNIT_IO_CHUNK;
	return cancellable ? __syscall_cp(SYS_write, fd, buf, count)
		: __syscall(SYS_write, fd, buf, count);
}
#endif
