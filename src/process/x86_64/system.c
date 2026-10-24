/* Dunit static profile has neither the signal/interruption contract required
 * by system() nor an agreed command-shell policy. Keep upstream implementation
 * in src/process/system.c; this backend explicitly refuses unavailable work. */
#include <stdlib.h>
#include <errno.h>
int system(const char *command)
{
    if (!command) return 0; /* no supported command processor available */
    errno = ENOSYS;
    return -1;
}
