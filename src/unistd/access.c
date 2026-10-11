#include <unistd.h>
#include <fcntl.h>

/* Dunit port: route access() through the Dunit faccessat() adapter (Stat-based
 * existence check) rather than a raw SYS_access/SYS_faccessat, which the kernel
 * does not implement. */
int access(const char *filename, int amode)
{
	return faccessat(AT_FDCWD, filename, amode, 0);
}
