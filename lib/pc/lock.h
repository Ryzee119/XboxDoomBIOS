// SPDX-License-Identifier: CC0-1.0

#include <stdatomic.h>
#include <stdlib.h>

#include "cpu.h"

#ifndef LOCK_H
#define LOCK_H
static inline void spinlock_acquire(atomic_flag *lock)
{
    while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire)) {
        system_yield(0);
    }
}

static inline void spinlock_release(atomic_flag *lock)
{
    atomic_flag_clear_explicit(lock, memory_order_release);
}

#endif
