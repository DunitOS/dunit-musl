#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "syscall.h"

/* Dunit port: open adapter.
 *
 * The kernel's Open syscall is Open(path_ptr, path_len, dunit_flags) — the path
 * is a pointer+length (not NUL-terminated) and the flags use Dunit's own bit
 * layout, NOT the POSIX/Linux values musl's headers define. So open cannot be a
 * plain syscall-number remap; the __sys_open* macros (src/internal/syscall.h)
 * funnel every opener (open/openat/fopen/posix_spawn/…) through here instead.
 *
 * Dunit OpenFlags bits — kernel/src/fs/vfs.rs (TODO: promote into the generated
 * abi/include/dunit headers so this stops being a hand-kept copy).
 */
#define DUNIT_O_READ   (1 << 0)
#define DUNIT_O_WRITE  (1 << 1)
#define DUNIT_O_CREATE (1 << 2)
#define DUNIT_O_TRUNC  (1 << 3)
#define DUNIT_O_APPEND (1 << 4)
#define DUNIT_O_EXCL   (1 << 5)

hidden int __dunit_oflags(int f)
{
	int d = 0;
	/* 64-bit off_t and no controlling tty make LARGEFILE/NOCTTY neutral.
	 * Other requested semantics need mechanisms, never silently dropped bits. */
	const int supported = O_ACCMODE|O_CREAT|O_TRUNC|O_APPEND|O_EXCL|O_LARGEFILE|O_NOCTTY|O_CLOEXEC|O_NONBLOCK;
	if (f & ~supported) return -EOPNOTSUPP;
	switch (f & O_ACCMODE) {
	case O_RDONLY: d |= DUNIT_O_READ; break;
	case O_WRONLY: d |= DUNIT_O_WRITE; break;
	case O_RDWR:   d |= DUNIT_O_READ | DUNIT_O_WRITE; break;
	default: return -EINVAL;
	}
	if (f & O_CREAT)  d |= DUNIT_O_CREATE;
	if (f & O_TRUNC)  d |= DUNIT_O_TRUNC;
	if (f & O_APPEND) d |= DUNIT_O_APPEND;
	if (f & O_EXCL) d |= DUNIT_O_EXCL;
	if (f & O_NONBLOCK) d |= 64;
	if (f & O_CLOEXEC) d |= 128;
	return d;
}

/* mode is accepted for POSIX source compatibility; Dunit Open has no mode arg. */
hidden long __dunit_open(const char *path, int flags, int mode)
{
	(void)mode;
	int d = __dunit_oflags(flags);
	if (d < 0) return d;
	return __syscall(SYS_open, path, strlen(path), d);
}

hidden long __dunit_open_cp(const char *path, int flags, int mode)
{
	(void)mode;
	int d = __dunit_oflags(flags);
	if (d < 0) return d;
	return __syscall_cp(SYS_open, path, strlen(path), d);
}
