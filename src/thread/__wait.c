#include "pthread_impl.h"

void __wait(volatile int *addr, volatile int *waiters, int val, int priv)
{
	int spins=100;
	if (priv) priv = FUTEX_PRIVATE;
	while (spins-- && (!waiters || !*waiters)) {
		if (*addr==val) a_spin();
		else return;
	}
	if (waiters) a_inc(waiters);
	while (*addr==val) {
		/* Dunit port: native FutexWait (timeout 0 = forever) instead of the
		 * Linux SYS_futex, so a contended waiter actually blocks in the kernel
		 * rather than busy-spinning on an ENOSYS. */
		__dunit_futex(addr, FUTEX_WAIT|priv, val, 0);
	}
	if (waiters) a_dec(waiters);
}
