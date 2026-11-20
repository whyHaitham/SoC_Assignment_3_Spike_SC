/* Minimal <sys/mman.h> for building MESS traffic_gen.c with newlib for pk.
 *
 * newlib (riscv64-unknown-elf) ships no <sys/mman.h>.  Rather than modifying
 * MESS's allocation code, mmap() here always fails, so traffic_gen.c takes its
 * own documented fallback path (posix_memalign, 64-byte aligned).
 */
#ifndef MESS_PK_COMPAT_SYS_MMAN_H
#define MESS_PK_COMPAT_SYS_MMAN_H

#include <stddef.h>
#include <sys/types.h>

#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED    ((void *)-1)

static inline void *mmap(void *addr, size_t len, int prot, int flags, int fd,
                         off_t off)
{
    (void)addr; (void)len; (void)prot; (void)flags; (void)fd; (void)off;
    return MAP_FAILED;
}

static inline int munmap(void *addr, size_t len)
{
    (void)addr; (void)len;
    return -1;
}

#endif
