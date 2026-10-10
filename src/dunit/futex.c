#include "pthread_impl.h"
#include "futex.h"
#include "syscall.h"

/* Dunit port: futex. Linux multiplexes wait/wake through one SYS_futex with the
 * operation in an argument; Dunit has two syscalls, FutexWait(addr, expected,
 * timeout_ms) and FutexWake(addr, count). This dispatcher is called from the
 * __wake/__futexwait inlines (pthread_impl.h) and the __timedwait wait path.
 * timeout_ms == 0 means wait forever (matches the kernel). */

#define DUNIT_SYS_FUTEX_WAIT 41   /* abi/syscalls.abi: FutexWait */
#define DUNIT_SYS_FUTEX_WAKE 42   /* abi/syscalls.abi: FutexWake */

hidden long __dunit_futex(volatile void *addr, int op, int val, long timeout_ms)
{
	switch (op & 0x7f) {   /* strip FUTEX_PRIVATE (128) / CLOCK bits */
	case FUTEX_WAIT:
		return __syscall(DUNIT_SYS_FUTEX_WAIT, addr, val, timeout_ms);
	case FUTEX_WAKE:
		return __syscall(DUNIT_SYS_FUTEX_WAKE, addr, val);
	default:
		return -ENOSYS;
	}
}

