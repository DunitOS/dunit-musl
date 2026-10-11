#include <poll.h>
#include <time.h>
#include "syscall.h"

/* Dunit port: the kernel Poll syscall is a NON-blocking readiness snapshot, so
 * the wait is implemented here — re-poll on a short sleep until something is
 * ready or the timeout elapses. timeout < 0 blocks indefinitely, 0 is a single
 * non-blocking check. (A fully event-driven kernel poll can replace this later;
 * the sleep yields the CPU meanwhile.) */
#define DUNIT_SYS_POLL 87
#define POLL_SLICE_MS 5

int poll(struct pollfd *fds, nfds_t n, int timeout)
{
	long r = __syscall(DUNIT_SYS_POLL, fds, (long)n);
	if (r != 0 || timeout == 0)
		return __syscall_ret(r);

	struct timespec start = { 0, 0 }, now;
	if (timeout > 0)
		clock_gettime(CLOCK_MONOTONIC, &start);

	for (;;) {
		struct timespec slice = { 0, POLL_SLICE_MS * 1000000L };
		nanosleep(&slice, 0);
		r = __syscall(DUNIT_SYS_POLL, fds, (long)n);
		if (r != 0)
			return __syscall_ret(r);
		if (timeout > 0) {
			clock_gettime(CLOCK_MONOTONIC, &now);
			long ms = (now.tv_sec - start.tv_sec) * 1000
				+ (now.tv_nsec - start.tv_nsec) / 1000000L;
			if (ms >= timeout)
				return 0;
		}
	}
}
