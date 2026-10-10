#include <sys/stat.h>
#include <string.h>
#include "syscall.h"

/* Dunit port: Mkdir takes (path_ptr, path_len); mode has no Dunit equivalent
 * yet and is dropped. */
int mkdir(const char *path, mode_t mode)
{
	(void)mode;
	return syscall(SYS_mkdir, path, strlen(path));
}
