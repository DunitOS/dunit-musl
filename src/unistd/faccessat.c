#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include "syscall.h"

/* Dunit port: access/faccessat.
 *
 * Dunit has no per-file permission model yet, so accessibility collapses to
 * existence: Stat the path and report success if it resolves, otherwise the
 * kernel's error (ENOENT/…). R_OK/W_OK/X_OK are therefore all treated as F_OK —
 * the honest best a permissionless VFS can answer. This is a documented
 * simplification, NOT a fake success (M6.1): a real check needs a kernel
 * permission model. Only AT_FDCWD / absolute paths are supported; dirfd-relative
 * access returns ENOSYS. Mirrors the Stat(28) wire struct in
 * kernel/src/syscall/mod.rs (UserFileStat, #[repr(C)]: u32 then usize). */
#define DUNIT_SYS_STAT 28

struct dunit_stat {
	uint32_t file_type;
	size_t size;
};

int faccessat(int fd, const char *path, int amode, int flag)
{
	(void)amode;
	(void)flag;
	if (fd != AT_FDCWD && path && path[0] != '/')
		return __syscall_ret(-ENOSYS);
	struct dunit_stat ds = { 0, 0 };
	long r = __syscall(DUNIT_SYS_STAT, path, strlen(path), &ds);
	return __syscall_ret(r < 0 ? r : 0);
}
