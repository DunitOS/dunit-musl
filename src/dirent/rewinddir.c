#include <dirent.h>
#include <unistd.h>
#include "__dirent.h"
#include "lock.h"

void __dunit_seekdir(DIR *, long);

void rewinddir(DIR *dir)
{
	LOCK(dir->lock);
	__dunit_seekdir(dir, 0);
	UNLOCK(dir->lock);
}
