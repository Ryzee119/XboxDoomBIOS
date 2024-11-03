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
