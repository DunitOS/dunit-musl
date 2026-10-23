#include <unistd.h>
#include "dunit_io.h"

ssize_t write(int fd, const void *buf, size_t count)
{
	return __syscall_ret(__dunit_write(fd, buf, count, 1));
}
