#include <unistd.h>
#include <string.h>
#include "syscall.h"

/* Dunit port: no separate rmdir; the native Unlink/remove_at handles (empty)
 * directories too. Pass (path_ptr, path_len). */
int rmdir(const char *path)
{
	return syscall(SYS_rmdir, path, strlen(path));
}
