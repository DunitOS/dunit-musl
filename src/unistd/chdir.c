#include <unistd.h>
#include <string.h>
#include "syscall.h"

/* Dunit port: the kernel's Chdir takes (path_ptr, path_len), not a
 * NUL-terminated pointer, so pass the length explicitly. */
int chdir(const char *path)
{
	return syscall(SYS_chdir, path, strlen(path));
}
