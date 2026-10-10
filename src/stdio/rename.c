#include <stdio.h>
#include <string.h>
#include "syscall.h"

/* Dunit port: Rename takes (old_ptr, old_len, new_ptr, new_len). */
int rename(const char *old, const char *new)
{
	return syscall(SYS_rename, old, strlen(old), new, strlen(new));
}
