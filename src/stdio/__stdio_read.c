#include "stdio_impl.h"
#include "dunit_io.h"

/* A native scalar read replaces readv, preserving FILE's pushback invariant:
 * after refill rpos/rend describe actual buffered bytes, and an empty buffer
 * starts at buf (so scanf can retain the last character in buf[-1]). */
size_t __stdio_read(FILE *f, unsigned char *buf, size_t len)
{
    f->rpos = f->rend = f->buf;
    int refill = len == 1 && f->buf_size;
    ssize_t cnt = __syscall_ret(__dunit_read(f->fd,
        refill ? f->buf : buf, refill ? f->buf_size : len, 0));
    if (cnt <= 0) {
        f->flags |= cnt ? F_ERR : F_EOF;
        return 0;
    }
    if (refill) {
        f->rend = f->buf + cnt;
        *buf = *f->rpos++;
        return 1;
    }
    return cnt;
}
