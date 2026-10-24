#include <spawn.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/* PATH is caller-supplied userspace policy. No implicit current directory or
 * compiled-in search path; literal paths are passed unchanged to spawn. */
int posix_spawnp(pid_t *restrict res, const char *restrict file,
	const posix_spawn_file_actions_t *fa,
	const posix_spawnattr_t *restrict attr,
	char *const argv[restrict], char *const envp[restrict])
{
	if (!file || !*file) return ENOENT;
	if (strchr(file, '/')) return posix_spawn(res, file, fa, attr, argv, envp);
	const char *path = getenv("PATH");
	if (!path) return ENOENT;
	int denied = 0;
	for (const char *p = path; *p;) {
		const char *end = strchr(p, ':');
		if (!end) end = p + strlen(p);
		size_t n = end-p, len = strlen(file);
		if (n && *p == '/') {
			if (n+len+2 > PATH_MAX) return ENAMETOOLONG;
			char full[PATH_MAX];
			memcpy(full,p,n); full[n]='/'; memcpy(full+n+1,file,len+1);
			int r = posix_spawn(res,full,fa,attr,argv,envp);
			if (r == EACCES) denied = 1;
			else if (r != ENOENT && r != ENOTDIR) return r;
		}
		p = *end ? end+1 : end;
	}
	return denied ? EACCES : ENOENT;
}
