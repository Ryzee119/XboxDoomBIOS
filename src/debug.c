#include "main.h"

static int _putc(char c, FILE *file)
{
    (void)file;
    xbox_serial_putchar(c);
    display_write_char(c);
    // smbus_output_byte(0x01 << 1, 0x0C, c);
    return c;
}

static FILE __stdio = FDEV_SETUP_STREAM(_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdin = &__stdio;
__strong_reference(stdin, stdout);
__strong_reference(stdin, stderr);

// Thread safe printf that is simply wrapped in a mutex
int printf_ts(const char *format, ...)
{
    static StaticSemaphore_t mutex;
    static SemaphoreHandle_t handle = NULL;
    if (handle == NULL) {
        handle = xSemaphoreCreateMutexStatic(&mutex);
    }

    xSemaphoreTake(handle, portMAX_DELAY);
    va_list args;
    va_start(args, format);
    int r = vprintf(format, args);
    va_end(args);
    xSemaphoreGive(handle);

    xbox_video_flush_cache();

    return r;
}
