#include <dirent.h>
#include <unistd.h>
#include "__dirent.h"
#include "lock.h"

void __dunit_seekdir(DIR *, long);

void seekdir(DIR *dir, long off)
{
	LOCK(dir->lock);
	__dunit_seekdir(dir, off);
	UNLOCK(dir->lock);
}
