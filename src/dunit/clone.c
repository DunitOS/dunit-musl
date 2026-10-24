#define _GNU_SOURCE
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include "pthread_impl.h"
#include "syscall.h"

/* Dunit port: thread creation.
 *
 * musl's x86_64 clone.s issues the Linux clone(2) and relies on CLONE_SETTLS to
 * install the child's thread pointer. Dunit instead has ThreadCreate(entry,
 * stack_top, arg) with no TLS argument, so we replace __clone in C: the parent
 * packs {fn,arg,tls,ctid} onto the top of the child stack and starts a
 * trampoline that installs the thread pointer FIRST (via the native
 * SetThreadPointer), registers the clear-child-tid word (CLONE_CHILD_CLEARTID,
 * which musl needs to release its thread-list lock across the final exit), then
 * runs musl's thread entry. clone.s is neutralized (this definition wins).
 */

#define DUNIT_SYS_THREAD_CREATE 29   /* abi/syscalls.abi: ThreadCreate */

struct dunit_thread {
	int (*fn)(void *);
	void *arg;
	void *tls;
	void *ctid;
};

__attribute__((no_stack_protector))
static _Noreturn void dunit_thread_start(void *p)
{
	struct dunit_thread *d = p;
	/* Must run before anything touches %fs (errno, stack guard, TLS vars). */
	__set_thread_area(d->tls);
	/* Register clear-child-tid (CLONE_CHILD_CLEARTID): the kernel zeroes this
	 * word and futex-wakes it when the thread exits, releasing the thread-list
	 * lock musl holds across SYS_exit. */
	if (d->ctid)
		__syscall(SYS_set_tid_address, d->ctid);
	d->fn(d->arg);
	/* musl's thread entry exits via SYS_exit itself; this is a safety net. */
	__syscall(SYS_exit, 0);
	for (;;) ;
}

/* Matches the variadic prototype in pthread_impl.h. pthread_create calls this as
 * __clone(fn, stack, flags, arg, &tid, TP_ADJ(td), &thread_list_lock). We use
 * fn/stack/arg/ptid/tls and ctid: the child registers ctid via SetTidAddress so
 * the kernel clears it + futex-wakes on thread exit (CLONE_CHILD_CLEARTID),
 * releasing the thread-list lock musl holds across SYS_exit. */
int __clone(int (*fn)(void *), void *stack, int flags, void *arg, ...)
{
	(void)flags;
	va_list ap;
	va_start(ap, arg);
	pid_t *ptid = va_arg(ap, pid_t *);
	void *tls = va_arg(ap, void *);
	void *ctid = va_arg(ap, void *);
	va_end(ap);

	uintptr_t sp = (uintptr_t)stack & -16;
	sp -= sizeof(struct dunit_thread);
	sp &= -16;
	struct dunit_thread *d = (struct dunit_thread *)sp;
	d->fn = fn;
	d->arg = arg;
	d->tls = tls;
	d->ctid = ctid;

	long tid = __syscall(DUNIT_SYS_THREAD_CREATE,
			     (long)(uintptr_t)dunit_thread_start, (long)sp, (long)(uintptr_t)d);
	if (tid >= 0) {
		((pthread_t)tls)->dunit_tid = (int)tid;
		if (ptid) *ptid = (pid_t)tid;
	}
	return (int)tid;
}
