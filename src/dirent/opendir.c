#define _GNU_SOURCE
#include <dirent.h>
#include "__dirent.h"

/* Dunit port: opendir snapshots the directory (see dunit_getdents.c), since the
 * kernel's Readdir is path-based rather than an fd+offset getdents(2). */
DIR *__dunit_opendir(const char *name);

DIR *opendir(const char *name)
{
	return __dunit_opendir(name);
}
