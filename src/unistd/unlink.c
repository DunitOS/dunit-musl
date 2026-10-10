#include <unistd.h>
#include <string.h>
#include "syscall.h"

/* Dunit port: Unlink takes (path_ptr, path_len). */
int unlink(const char *path)
{
	return syscall(SYS_unlink, path, strlen(path));
}
