#define _GNU_SOURCE
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "__dirent.h"
#include "syscall.h"

/* Dunit port: directory iteration.
 *
 * The kernel's Readdir is path-based — Readdir(path_ptr, path_len, *UserDirEntry,
 * capacity) fills an array of entries in one shot — not the fd+offset
 * getdents(2) model musl expects. So opendir() snapshots the whole directory
 * here, and getdents() just serializes the snapshot into the struct dirent
 * records readdir() parses. There is no real kernel fd (dir->fd = -1); dirfd()
 * is therefore not usable as a descriptor (documented).
 *
 * struct + type codes mirror kernel/src/syscall/mod.rs (UserDirEntry #[repr(C)];
 * user_file_type()). */
#define DUNIT_SYS_READDIR 27
#define DUNIT_FT_FILE 1
#define DUNIT_FT_DIR  2
#define DUNIT_FT_DEV  3
#define DUNIT_DIR_CAP 256

struct dunit_dirent {
	char name[64];
	size_t name_len;
	uint32_t file_type;
};

struct dunit_dir {
	struct __dirstream ds;   /* first member: DIR* aliases &ds */
	struct dunit_dirent *ents;
	int count;
	int pos;
};

DIR *__dunit_opendir(const char *name)
{
	struct dunit_dir *dd = calloc(1, sizeof *dd);
	if (!dd) return 0;
	dd->ents = calloc(DUNIT_DIR_CAP, sizeof *dd->ents);
	if (!dd->ents) { free(dd); return 0; }

	long n = __syscall(DUNIT_SYS_READDIR, name, strlen(name),
			   dd->ents, (long)DUNIT_DIR_CAP);
	if (n < 0) {
		errno = (int)-n;
		free(dd->ents);
		free(dd);
		return 0;
	}
	dd->count = (int)n;
	dd->ds.fd = -1;   /* snapshot-backed; no real kernel fd */
	return &dd->ds;
}

int __dunit_closedir(DIR *dir)
{
	struct dunit_dir *dd = (struct dunit_dir *)dir;
	free(dd->ents);
	free(dd);
	return 0;
}

static unsigned char dt_from_dunit(uint32_t t)
{
	switch (t) {
	case DUNIT_FT_DIR:  return DT_DIR;
	case DUNIT_FT_DEV:  return DT_CHR;
	case DUNIT_FT_FILE: return DT_REG;
	default:            return DT_UNKNOWN;
	}
}

int __dunit_getdents(DIR *dir, char *buf, int len)
{
	struct dunit_dir *dd = (struct dunit_dir *)dir;
	int used = 0;

	while (dd->pos < dd->count) {
		struct dunit_dirent *e = &dd->ents[dd->pos];
		size_t nl = e->name_len < 63 ? e->name_len : 63;
		size_t reclen = offsetof(struct dirent, d_name) + nl + 1;
		reclen = (reclen + sizeof(off_t) - 1) & ~(sizeof(off_t) - 1);
		if (used + (int)reclen > len)
			break;
		struct dirent *de = (struct dirent *)(buf + used);
		de->d_ino = dd->pos + 1;
		de->d_off = dd->pos + 1;
		de->d_reclen = (unsigned short)reclen;
		de->d_type = dt_from_dunit(e->file_type);
		memcpy(de->d_name, e->name, nl);
		de->d_name[nl] = '\0';
		used += (int)reclen;
		dd->pos++;
	}
	return used;
}
