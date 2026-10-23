#include <stdio.h>
#include <errno.h>
#include <unistd.h>

/* Use the Dunit path adapters: native Unlink/Rmdir also require path length. */
int remove(const char *path)
{
	int result = unlink(path);
	if (result < 0 && errno == EISDIR) result = rmdir(path);
	return result;
}
