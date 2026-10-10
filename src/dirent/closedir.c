#include <dirent.h>
#include "__dirent.h"

/* Dunit port: frees the snapshot allocated by opendir (dunit_getdents.c); there
 * is no real kernel fd to close. */
int __dunit_closedir(DIR *dir);

int closedir(DIR *dir)
{
	return __dunit_closedir(dir);
}
