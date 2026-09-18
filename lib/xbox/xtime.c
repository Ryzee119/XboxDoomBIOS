// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

// https://wiki.osdev.org/ACPI_Timer
// On Xbox this is located at 0x8008

uint32_t xbox_timer_query_performance_frequency(void)
{
    return ACPI_TIMER_FREQ;
}

uint32_t xbox_timer_query_performance_counter(void)
{
    return io_input_dword(XBOX_ACPI_TIMER_PORT) & 0x00FFFFFF;
}

void xbox_timer_spin_wait(uint32_t ticks)
{
    uint32_t start = xbox_timer_query_performance_counter();
    while (((xbox_timer_query_performance_counter() - start) & 0x00FFFFFF) < ticks)
        ;
}

// xbox_get_time