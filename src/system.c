#include "main.h"

extern uint8_t freertos_running;

void system_yield(uint32_t ms)
{
    if (freertos_running) {
        if (ms == 0) {
            taskYIELD();
        } else {
            vTaskDelay(pdMS_TO_TICKS(ms));
        }
    } else {
        xbox_timer_spin_wait(XBOX_TIMER_MS_TO_TICKS(ms));
    }
}

uint32_t system_tick(void)
{
    if (freertos_running) {
        return xTaskGetTickCount();
    } else {
        // Not ideal as can wrap around reasonbly quickly but okay until we get FreeRTOS ticking
        uint32_t htick = xbox_timer_query_performance_counter();
        uint64_t intermediate = (uint64_t)htick * 1000ULL;
        uint32_t system_tick = (uint32_t)(intermediate / xbox_timer_query_performance_frequency());
        return system_tick;
    }
}

void *system_get_physical_address(void *virtual_address)
{
    return virtual_address;
}

#include <sys/lock.h>

struct __lock
{
    char unused;
};

struct __lock __lock___libc_recursive_mutex;

void __retarget_lock_init(_LOCK_T *lock)
{
    (void)lock;
}

void __retarget_lock_init_recursive(_LOCK_T *lock)
{
    (void)lock;
}

void __retarget_lock_close(_LOCK_T lock)
{
    (void)lock;
}

void __retarget_lock_close_recursive(_LOCK_T lock)
{
    (void)lock;
}

void __no_thread_safety_analysis __retarget_lock_acquire(_LOCK_T lock)
{
    (void)lock;
    if (freertos_running) {
        vTaskSuspendAll();
    }
}

void __no_thread_safety_analysis __retarget_lock_acquire_recursive(_LOCK_T lock)
{
    (void)lock;
    if (freertos_running) {
        vTaskSuspendAll();
    }
}

void __no_thread_safety_analysis __retarget_lock_release(_LOCK_T lock)
{
    (void)lock;
    if (freertos_running) {
        (void)xTaskResumeAll();
    }
}

void __no_thread_safety_analysis __retarget_lock_release_recursive(_LOCK_T lock)
{
    (void)lock;
    if (freertos_running) {
        (void)xTaskResumeAll();
    }
}
