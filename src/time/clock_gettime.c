#include <time.h>
#include "syscall.h"

/* Dunit port: one native clock syscall writes {tv_sec, tv_nsec} (two u64, the
 * x86_64 timespec layout). Dunit has no RTC yet, so CLOCK_MONOTONIC (1) is the
 * uptime since boot and CLOCK_REALTIME (0) returns the same monotonic value —
 * documented, not faked as wall-clock. Unknown clocks get EINVAL from the
 * kernel. This replaces upstream's VDSO/time64/gettimeofday fallback chain. */
#define DUNIT_SYS_CLOCK_GETTIME 76

int __clock_gettime(clockid_t clk, struct timespec *ts)
{
	return __syscall_ret(__syscall(DUNIT_SYS_CLOCK_GETTIME, clk, ts));
}

weak_alias(__clock_gettime, clock_gettime);
