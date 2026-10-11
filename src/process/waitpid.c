#include <sys/wait.h>
#include <errno.h>
#include "syscall.h"

/* Dunit port: wait via the native WaitProcess syscall, which is non-blocking
 * (returns -EAGAIN while the child still runs) and writes {kind, code}. We poll
 * with sched_yield between tries and translate to a POSIX wait status: kind 0 is
 * a normal exit (WEXITSTATUS = code); a fault maps to the matching signal. */
#define DUNIT_SYS_WAIT  16
#define DUNIT_SYS_YIELD 24

struct dunit_wait { int kind; int code; };

pid_t waitpid(pid_t pid, int *status, int options)
{
	unsigned dpid = (pid > 0) ? (unsigned)pid : 0;   /* 0 = any child */
	for (;;) {
		struct dunit_wait w = { 0, 0 };
		long r = __syscall(DUNIT_SYS_WAIT, dpid, &w);
		if (r == -EAGAIN) {
			if (options & WNOHANG) return 0;
			__syscall(DUNIT_SYS_YIELD);
			continue;
		}
		if (r < 0) { errno = (int)-r; return -1; }
		if (status) {
			int st;
			if (w.kind == 0) {
				st = (w.code & 0xff) << 8;      /* WIFEXITED */
			} else {
				int sig;
				switch (w.kind) {
				case 1: case 2: sig = 11; break; /* page/GP fault -> SIGSEGV */
				case 3: sig = 4;  break;         /* bad opcode   -> SIGILL  */
				case 4: sig = 8;  break;         /* div-by-zero  -> SIGFPE  */
				default: sig = 9; break;         /*              -> SIGKILL */
				}
				st = sig;                        /* WIFSIGNALED */
			}
			*status = st;
		}
		return (pid_t)r;
	}
}
