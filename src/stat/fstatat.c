#define _BSD_SOURCE
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include "syscall.h"

/* Dunit port: stat.
 *
 * The kernel's Stat is Stat(path_ptr, path_len, *UserFileStat) and reports only
 * a file type + size (no times, ids, inode, nlink). We translate that into a
 * struct stat: st_size and st_mode's type bits are real; the rest stay zero
 * (not faked — just unavailable). Dunit has no stat-by-fd, so fstat
 * (AT_EMPTY_PATH) and dirfd-relative paths report a documented ENOSYS.
 *
 * struct layout + type codes must match kernel/src/syscall/mod.rs
 * (UserFileStat, #[repr(C)]: u32 then usize; user_file_type()). */
#define DUNIT_SYS_STAT 28          /* abi/syscalls.abi: Stat */
#define DUNIT_FT_FILE  1
#define DUNIT_FT_DIR   2
#define DUNIT_FT_DEV   3

struct dunit_stat {
	uint32_t file_type;
	size_t size;
};

int __fstatat(int fd, const char *restrict path, struct stat *restrict st, int flag)
{
	if (((flag & AT_EMPTY_PATH) && path && !*path) ||
	    (fd != AT_FDCWD && path && path[0] != '/')) {
		/* by-fd / dirfd-relative stat is not supported by the kernel. */
		return __syscall_ret(-ENOSYS);
	}

	struct dunit_stat ds = { 0, 0 };
	long r = __syscall(DUNIT_SYS_STAT, path, strlen(path), &ds);
	if (r < 0)
		return __syscall_ret(r);

	mode_t mode;
	switch (ds.file_type) {
	case DUNIT_FT_DIR: mode = S_IFDIR | 0755; break;
	case DUNIT_FT_DEV: mode = S_IFCHR | 0666; break;
	default:           mode = S_IFREG | 0644; break;
	}

	*st = (struct stat){
		.st_mode = mode,
		.st_nlink = 1,
		.st_size = (off_t)ds.size,
		.st_blksize = 4096,
		.st_blocks = (blkcnt_t)((ds.size + 511) / 512),
	};
	return __syscall_ret(0);
}

weak_alias(__fstatat, fstatat);
