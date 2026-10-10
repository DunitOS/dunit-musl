#include <time.h>
#include <errno.h>
#include "syscall.h"

/* Dunit port: sleep via the native Sleep(ms) syscall (relative, tick-granular).
 * Converts the timespec to milliseconds (rounding sub-ms up to 1ms). For
 * TIMER_ABSTIME we subtract the current clock to get a relative delay. Sleeps
 * are not interruptible on Dunit yet, so this never returns EINTR/rem. */
#define DUNIT_SYS_SLEEP 19
#define DUNIT_SYS_CLOCK_GETTIME 76

int __clock_nanosleep(clockid_t clk, int flags, const struct timespec *req, struct timespec *rem)
{
	(void)rem;
	long long sec = req->tv_sec;
	long nsec = req->tv_nsec;
	if (nsec < 0 || nsec >= 1000000000L)
		return EINVAL;

	if (flags & TIMER_ABSTIME) {
		struct timespec now = { 0, 0 };
		__syscall(DUNIT_SYS_CLOCK_GETTIME, clk, &now);
		sec -= now.tv_sec;
		nsec -= now.tv_nsec;
		if (nsec < 0) { nsec += 1000000000L; sec--; }
		if (sec < 0)
			return 0;  /* deadline already in the past */
	}

	long long ms = sec * 1000 + (nsec + 999999) / 1000000;
	if (ms < 0)
		return EINVAL;
	if (ms > 0)
		__syscall(DUNIT_SYS_SLEEP, ms);
	return 0;
}

weak_alias(__clock_nanosleep, clock_nanosleep);
