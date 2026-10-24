#include <sys/select.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>

/* Dunit port: select implemented on top of poll (which the kernel Poll syscall
 * backs). Build a pollfd set from the fd_sets, poll, then rebuild the sets from
 * the revents. `tv` is not updated with the remaining time (POSIX-optional;
 * documented). */
int select(int n, fd_set *restrict rfds, fd_set *restrict wfds,
	   fd_set *restrict efds, struct timeval *restrict tv)
{
	if (n < 0 || n > FD_SETSIZE) {
		errno = EINVAL;
		return -1;
	}
	struct pollfd pfd[FD_SETSIZE];
	int cnt = 0;
	for (int fd = 0; fd < n; fd++) {
		short ev = 0;
		if (rfds && FD_ISSET(fd, rfds)) ev |= POLLIN;
		if (wfds && FD_ISSET(fd, wfds)) ev |= POLLOUT;
		if (efds && FD_ISSET(fd, efds)) ev |= POLLPRI;
		if (ev) {
			pfd[cnt].fd = fd;
			pfd[cnt].events = ev;
			pfd[cnt].revents = 0;
			cnt++;
		}
	}

    if (tv && (tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000)) { errno=EINVAL; return -1; }
    if (tv && tv->tv_sec > (INT_MAX-1000)/1000) { errno=EOVERFLOW; return -1; }
    int timeout = tv ? (int)(tv->tv_sec*1000 + (tv->tv_usec+999)/1000) : -1;
	int r = poll(pfd, cnt, timeout);
	if (r < 0)
		return -1; /* poll set errno */

    for (int i=0;i<cnt;i++) if(pfd[i].revents&POLLNVAL){errno=EBADF;return -1;}
	if (rfds) FD_ZERO(rfds);
	if (wfds) FD_ZERO(wfds);
	if (efds) FD_ZERO(efds);
	int ready = 0;
	for (int i = 0; i < cnt; i++) {
		int fd = pfd[i].fd;
		short re = pfd[i].revents;
		int hit = 0;
		if (rfds && (re & (POLLIN | POLLHUP | POLLERR))) { FD_SET(fd, rfds); hit = 1; }
		if (wfds && (re & (POLLOUT | POLLERR))) { FD_SET(fd, wfds); hit = 1; }
		if (efds && (re & POLLPRI)) { FD_SET(fd, efds); hit = 1; }
		if (hit) ready++;
	}
	return ready;
}
