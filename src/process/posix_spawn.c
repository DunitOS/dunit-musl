#define _GNU_SOURCE
#include <spawn.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "syscall.h"
#include "fdop.h"
#include <stdint.h>
extern hidden int __dunit_oflags(int);

/* Dunit port: posix_spawn maps directly onto the native SpawnArgs syscall — no
 * clone/vfork+exec. argv and envp are packed into one NUL-terminated-string
 * block (argv first, then envp); the kernel loads the ELF with that argv/env
 * and schedules it, returning the child pid. Unsupported actions/attributes
 * fail before creating a child; never mutate the parent's descriptor table. */
#define DUNIT_SYS_SPAWN_ARGS 79

extern char **__environ;

static int dunit_spawn(pid_t *restrict res, const char *path,
	const posix_spawn_file_actions_t *fa,
	const posix_spawnattr_t *attr,
	char *const argv[], char *const envp[])
{
	if (!res || !path || !argv || !argv[0]) return EINVAL;
	if (attr && (attr->__flags || attr->__fn)) return ENOSYS;
	if (!envp) envp = __environ;

	int argc = 0;
	size_t total = 0;
	while (argv && argv[argc]) {
		size_t n = strlen(argv[argc]) + 1;
		if (n > 65536 - total) return E2BIG;
		total += n; argc++;
	}
	int envc = 0;
	while (envp && envp[envc]) {
		size_t n = strlen(envp[envc]) + 1;
		if (n > 65536 - total) return E2BIG;
		total += n; envc++;
	}

    size_t string_bytes=total;
    struct fdop *last=fa ? fa->__actions : 0;
    unsigned count=0;
    while(last && last->next) last=last->next;
    for(struct fdop *op=last;op;op=op->prev) {
        if(++count>64)return E2BIG;
        if(op->cmd==FDOP_FCHDIR)return ENOTSUP;
        if(op->cmd<FDOP_CLOSE || op->cmd>FDOP_CHDIR)return EINVAL;
        size_t length=(op->cmd==FDOP_OPEN || op->cmd==FDOP_CHDIR)?strlen(op->path):0;
        if(length>255 || total>65536-24-length)return E2BIG;
        if(op->cmd==FDOP_OPEN && __dunit_oflags(op->oflag)<0)return EOPNOTSUPP;
        total+=24+length;
    }
    char *block = malloc(total ? total : 1);
	if (!block) return ENOMEM;
	size_t off = 0;
	for (int i = 0; i < argc; i++) { size_t n = strlen(argv[i]) + 1; memcpy(block + off, argv[i], n); off += n; }
	for (int i = 0; i < envc; i++) { size_t n = strlen(envp[i]) + 1; memcpy(block + off, envp[i], n); off += n; }

    for(struct fdop *op=last;op;op=op->prev) {
        uint32_t words[6]={op->cmd,0,0,0,0,0};
        size_t length=(op->cmd==FDOP_OPEN || op->cmd==FDOP_CHDIR)?strlen(op->path):0;
        if(op->cmd!=FDOP_CHDIR)words[1]=op->fd;
        if(op->cmd==FDOP_DUP2)words[2]=op->srcfd;
        if(op->cmd==FDOP_OPEN){words[3]=__dunit_oflags(op->oflag);words[4]=op->mode;}
        words[5]=length;
        memcpy(block+off,words,24); off+=24;
        if(length){memcpy(block+off,op->path,length);off+=length;}
    }
    long r = __syscall(DUNIT_SYS_SPAWN_ARGS, path, strlen(path), block, off | (1UL<<63), (long)argc, string_bytes);
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
