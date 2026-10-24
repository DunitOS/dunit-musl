/* Native shell execution policy is not yet defined. The full upstream source
 * is retained at src/stdio/popen.c and must not be mistaken for a Dunit port. */
#include <stdio.h>
#include <errno.h>
FILE *popen(const char *command, const char *mode)
{
    (void)command;
    if (!mode || (mode[0] != 'r' && mode[0] != 'w') ||
        (mode[1] && (mode[1] != 'e' || mode[2]))) {
        errno = EINVAL;
        return 0;
    }
    errno = ENOSYS;
    return 0;
}
