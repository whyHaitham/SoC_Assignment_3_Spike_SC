/* POSIX functions used by MESS traffic_gen.c that bare-metal newlib (as used
 * with the RISC-V proxy kernel) declares but does not implement.
 *
 *  - posix_memalign(): the allocation path traffic_gen.c falls back to when
 *    mmap() fails (always, see pk_compat/sys/mman.h).  Implemented with
 *    newlib's memalign(); free() releases it as usual.
 *  - clock_gettime(): only used by the --self-benchmark mode, which is never
 *    used under Spike.  Implemented with gettimeofday(), which pk supports.
 */
#include <errno.h>
#include <malloc.h>
#include <stddef.h>
#include <sys/time.h>
#include <time.h>

int posix_memalign(void **out, size_t alignment, size_t size)
{
    void *p = memalign(alignment, size);
    if (!p)
        return ENOMEM;
    *out = p;
    return 0;
}

int clock_gettime(clockid_t clock_id, struct timespec *ts)
{
    struct timeval tv;
    (void)clock_id;
    if (gettimeofday(&tv, NULL) != 0)
        return -1;
    ts->tv_sec = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000L;
    return 0;
}
