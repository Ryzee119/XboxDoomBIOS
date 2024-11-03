#ifndef MAIN_H
#define MAIN_H

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include <FreeRTOS.h>
#include <freertos_irq.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include <timers.h>

#include <xbox/xbox.h>

#include <font/unscii_16.h>
#include <tinyusb/src/tusb.h>
#include <tusb_xinput/xinput_host.h>

#include "fileio.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define THREAD_PRIORITY_LOWEST  0
#define THREAD_PRIORITY_LOW     1
#define THREAD_PRIORITY_NORMAL  2
#define THREAD_PRIORITY_HIGH    3
#define THREAD_PRIORITY_HIGHEST 4

#if THREAD_PRIORITY_HIGHEST > configMAX_PRIORITIES
#error "THREAD_PRIORITY_HIGHEST is higher than configMAX_PRIORITIES"
#endif

int printf_ts(const char *format, ...);

void display_init(void);
void display_write_char(const char c);
void display_clear(void);
void display_get_cursor(uint32_t *x, uint32_t *y);
void display_set_cursor(uint32_t x, uint32_t y);
void display_write_char(const char c);
void display_write_char_ex(const char c, uint16_t x, uint16_t y, void *frame_buffer);

void usb_init(void);
void interrupts_init(void);

int doom_entry(const char *wad_path);
void dooom_new_input(uint16_t buttons, int16_t lx, int16_t ly, int16_t rx, int16_t ry, uint8_t lt, uint8_t rt);
void doom_sound_task(void *parameters);

void *pvPortMallocAligned(size_t xWantedSize, size_t xWantedAlignment);
void pvPortMallocAlignedFree(void *pv);
#endif
