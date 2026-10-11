#define _GNU_SOURCE
#include <spawn.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "syscall.h"

/* Dunit port: posix_spawn maps directly onto the native SpawnArgs syscall — no
 * clone/vfork+exec. argv and envp are packed into one NUL-terminated-string
 * block (argv first, then envp); the kernel loads the ELF with that argv/env
 * and schedules it, returning the child pid. File actions and spawn attributes
 * are not supported yet and are ignored (documented M6-D limitation). */
#define DUNIT_SYS_SPAWN_ARGS 79

extern char **__environ;

static int dunit_spawn(pid_t *restrict res, const char *path,
	const posix_spawn_file_actions_t *fa,
	const posix_spawnattr_t *attr,
	char *const argv[], char *const envp[])
{
	(void)fa;
	(void)attr;
	if (!envp) envp = __environ;

	int argc = 0;
	size_t total = 0;
	while (argv && argv[argc]) { total += strlen(argv[argc]) + 1; argc++; }
	int envc = 0;
	while (envp && envp[envc]) { total += strlen(envp[envc]) + 1; envc++; }

	char *block = malloc(total ? total : 1);
	if (!block) return ENOMEM;
	size_t off = 0;
	for (int i = 0; i < argc; i++) { size_t n = strlen(argv[i]) + 1; memcpy(block + off, argv[i], n); off += n; }
	for (int i = 0; i < envc; i++) { size_t n = strlen(envp[i]) + 1; memcpy(block + off, envp[i], n); off += n; }

	long r = __syscall(DUNIT_SYS_SPAWN_ARGS, path, strlen(path), block, off, (long)argc);
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
