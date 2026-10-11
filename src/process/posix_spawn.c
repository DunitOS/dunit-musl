#define _GNU_SOURCE
#include <spawn.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "syscall.h"
#include "fdop.h"

/* Dunit port: posix_spawn maps directly onto the native SpawnArgs syscall — no
 * clone/vfork+exec. The spawn block is [argv+env NUL-strings | serialized file
 * actions]; `string_len` (the 6th syscall arg) marks where the strings end. The
 * kernel loads the ELF with that argv/env, has the child inherit the parent's
 * fds, applies the file actions (open/dup2/close/chdir) to the child's table,
 * and schedules it. Spawn attributes are not supported and are refused with a
 * documented ENOSYS (M6.1 — never silently ignored). */
#define DUNIT_SYS_SPAWN_ARGS 79
#define FA_HDR 24   /* 6 × u32 header before each action's path bytes */

hidden int __dunit_oflags(int);

extern char **__environ;

static size_t fa_path_len(const struct fdop *op)
{
	if (op->cmd == FDOP_OPEN || op->cmd == FDOP_CHDIR)
		return strlen(op->path);
	return 0;
}

static void put32(unsigned char *p, uint32_t v)
{
	p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24;
}

// PLACEHOLDER

static int dunit_spawn(pid_t *restrict res, const char *path,
	const posix_spawn_file_actions_t *fa,
	const posix_spawnattr_t *attr,
	char *const argv[], char *const envp[])
{
	if (attr && attr->__flags) return ENOSYS;   /* attributes unsupported */
	if (!envp) envp = __environ;

	int argc = 0;
	size_t slen = 0;                             /* string-section byte length */
	while (argv && argv[argc]) { slen += strlen(argv[argc]) + 1; argc++; }
	int envc = 0;
	while (envp && envp[envc]) { slen += strlen(envp[envc]) + 1; envc++; }

	/* The action list is prepended (head = last added); walk to the tail so we
	 * can emit in addition order (tail → head via prev). */
	size_t falen = 0;
	const struct fdop *tail = 0;
	if (fa) {
		for (const struct fdop *op = fa->__actions; op; op = op->next) {
			falen += FA_HDR + fa_path_len(op);
			tail = op;
		}
	}

	size_t total = slen + falen;
	char *block = malloc(total ? total : 1);
	if (!block) return ENOMEM;
	size_t off = 0;
	for (int i = 0; i < argc; i++) { size_t n = strlen(argv[i]) + 1; memcpy(block + off, argv[i], n); off += n; }
	for (int i = 0; i < envc; i++) { size_t n = strlen(envp[i]) + 1; memcpy(block + off, envp[i], n); off += n; }

	for (const struct fdop *op = tail; op; op = op->prev) {
		unsigned char *p = (unsigned char *)block + off;
		size_t pl = fa_path_len(op);
		uint32_t fd    = (op->cmd == FDOP_CHDIR) ? 0 : (uint32_t)op->fd;
		uint32_t srcfd = (op->cmd == FDOP_DUP2)  ? (uint32_t)op->srcfd : 0;
		uint32_t flags = (op->cmd == FDOP_OPEN)  ? (uint32_t)__dunit_oflags(op->oflag) : 0;
		uint32_t mode  = (op->cmd == FDOP_OPEN)  ? (uint32_t)op->mode : 0;
		put32(p + 0,  (uint32_t)op->cmd);
		put32(p + 4,  fd);
		put32(p + 8,  srcfd);
		put32(p + 12, flags);
		put32(p + 16, mode);
		put32(p + 20, (uint32_t)pl);
		if (pl) memcpy(p + FA_HDR, op->path, pl);
		off += FA_HDR + pl;
	}

	long r = __syscall(DUNIT_SYS_SPAWN_ARGS, path, strlen(path), block, total, (long)argc, slen);
	free(block);
	if (r < 0) return (int)-r;   /* posix_spawn returns the error number */
	if (res) *res = (pid_t)r;
	return 0;
}

int posix_spawn(pid_t *restrict res, const char *restrict path,
	const posix_spawn_file_actions_t *fa,
	const posix_spawnattr_t *restrict attr,
	char *const argv[restrict], char *const envp[restrict])
{
	return dunit_spawn(res, path, fa, attr, argv, envp);
}

