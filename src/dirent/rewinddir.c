#include <dirent.h>
#include <unistd.h>
#include "__dirent.h"
#include "lock.h"

/* Dunit port: directories are snapshot-backed (see dunit_getdents.c), with the
 * iteration cursor in the snapshot, not an fd offset. Reset that cursor as well
 * as the readdir parse buffer; the lseek(dir->fd) the generic version does is a
 * no-op here (dir->fd = -1). */
void __dunit_rewinddir(DIR *dir);

void rewinddir(DIR *dir)
{
	LOCK(dir->lock);
	__dunit_rewinddir(dir);
	dir->buf_pos = dir->buf_end = 0;
	dir->tell = 0;
	UNLOCK(dir->lock);
}
