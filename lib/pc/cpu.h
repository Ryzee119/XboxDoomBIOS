// SPDX-License-Identifier: CC0-1.0

#ifndef IA32_CPU_H
#define IA32_CPU_H

#include <stdint.h>

#ifndef __BYTE_ORDER__
#error "Unknown byte order"
#endif

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define BSWAP_LE16(x) (x)
#define BSWAP_LE32(x) (x)
#define BSWAP_LE64(x) (x)
#define BSWAP_BE16(x) __builtin_bswap16(x)
#define BSWAP_BE32(x) __builtin_bswap32(x)
#define BSWAP_BE64(x) __builtin_bswap64(x)
#define BSWAP_BE16_TO_NATIVE(x) __builtin_bswap16(x)
#define BSWAP_BE32_TO_NATIVE(x) __builtin_bswap32(x)
#define BSWAP_BE64_TO_NATIVE(x) __builtin_bswap64(x)
#define BSWAP_LE16_TO_NATIVE(x) (x)
#define BSWAP_LE32_TO_NATIVE(x) (x)
#define BSWAP_LE64_TO_NATIVE(x) (x)
#else
#define BSWAP_LE16(x) __builtin_bswap16(x)
#define BSWAP_LE32(x) __builtin_bswap32(x)
#define BSWAP_LE64(x) __builtin_bswap64(x)
#define BSWAP_BE16(x) (x)
#define BSWAP_BE32(x) (x)
#define BSWAP_BE64(x) (x)
#define BSWAP_BE16_TO_NATIVE(x) (x)
#define BSWAP_BE32_TO_NATIVE(x) (x)
#define BSWAP_BE64_TO_NATIVE(x) (x)
#define BSWAP_LE16_TO_NATIVE(x) __builtin_bswap16(x)
#define BSWAP_LE32_TO_NATIVE(x) __builtin_bswap32(x)
#define BSWAP_LE64_TO_NATIVE(x) __builtin_bswap64(x)
#endif

static inline __attribute__((always_inline)) void cpu_read_msr(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

static inline __attribute__((always_inline)) void cpu_write_msr(uint32_t msr, uint32_t lo, uint32_t hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

static inline __attribute__((always_inline)) void cpu_disable_cache(void)
{
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1 << 30);
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline __attribute__((always_inline)) void cpu_enable_cache(void)
{
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1 << 30);
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

static inline __attribute__((always_inline)) void cpu_read_cpuid(uint32_t code, uint32_t *eax, uint32_t *ebx,
                                                                 uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
}

static inline __attribute__((always_inline)) void cpu_read_msr64(uint32_t msr, uint64_t *p)
{
    uint32_t __lo, __hi;
    cpu_read_msr(msr, &__lo, &__hi);
    *(p) = ((uint64_t)__hi << 32) | __lo;
}

static inline __attribute__((always_inline)) void cpu_write_msr64(uint32_t msr, uint64_t v)
{
    uint32_t __lo = (uint32_t)(v);
    uint32_t __hi = (uint32_t)((v) >> 32);
    cpu_write_msr(msr, __lo, __hi);
}

void system_yield(uint32_t ms);
uint32_t system_tick(void);
void *system_get_physical_address(void *ptr);

#endif // IA32_CPU_H