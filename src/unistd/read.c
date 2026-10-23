#include <unistd.h>
#include "dunit_io.h"

ssize_t read(int fd, void *buf, size_t count)
{
	return __syscall_ret(__dunit_read(fd, buf, count, 1));
}
