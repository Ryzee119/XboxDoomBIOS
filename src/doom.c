#define DOOM_IMPLEMENTATION
#include "main.h"

// Wtf doom
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wsometimes-uninitialized"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wparentheses"
#include <PureDOOM/PureDOOM.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

static SemaphoreHandle_t doom_logic_mutex;
static SemaphoreHandle_t doom_vblank_semaphore;

uint8_t doom_initd = 0;
int16_t doom_rightx = 0;

static void dooom_printf(const char *str)
{
    printf_ts("%s", str);
}

static void *dooom_malloc(int size)
{
    return pvPortMalloc(size);
}

static void dooom_free(void *ptr)
{
    vPortFree(ptr);
}

static void dooom_gettime(int *sec, int *usec)
{
    static uint32_t base_ms = 0;
    if (base_ms == 0) {
        base_ms = pdTICKS_TO_MS(xTaskGetTickCount());
    }
    int ms = pdTICKS_TO_MS(xTaskGetTickCount()) - base_ms;
    *sec = ms / 1000;
    *usec = (ms % 1000) * 1000;
}

static void *dooom_open(const char *filename, const char *mode)
{
    if (filename == NULL) {
        return NULL;
    }

    return fopen(filename, mode);
}

static void dooom_close(void *handle)
{
    fclose((FILE *)handle);
}

static int dooom_read(void *handle, void *buf, int count)
{
    return fread(buf, 1, count, (FILE *)handle);
}

static int dooom_write(void *handle, const void *buf, int count)
{
    return fwrite(buf, 1, count, (FILE *)handle);
}

static int dooom_seek(void *handle, int offset, doom_seek_t origin)
{
    return fseek(handle, offset, origin);
}

static int dooom_tell(void *handle)
{
    return ftell((FILE *)handle);
}

static int dooom_eof(void *handle)
{
    return feof((FILE *)handle);
}

void *doom_memcpy(void *destination, const void *source, int num)
{
    return memcpy(destination, source, num);
}

void doom_memset(void *ptr, int value, int num)
{
    memset(ptr, value, num);
}

static char wad_dir[256];
char *dooom_getenv(const char *var)
{
    if (strcmp(var, "DOOMWADDIR") == 0) {
        return "E:/doom";
    }
    if (strcmp(var, "HOME") == 0) {
        return "E:/doom";
    }
    printf_ts("[DOOM] Unknown env var %s\n", var);
    return 0;
}

static void vblank_callback()
{
    xSemaphoreGiveFromISR(doom_vblank_semaphore, NULL);
}

void scale_doom(const uint8_t *input, int inputWidth, int inputHeight, uint32_t *output, int outputWidth,
                int outputHeight)
{
    float scaleX = (float)outputWidth / inputWidth;
    float scaleY = (float)outputHeight / (inputHeight);

    float scale_y = (scaleX < scaleY) ? scaleX : scaleY;
    float scale_x = scale_y;

    int scaledWidth = (int)(inputWidth * scale_x);
    int scaledHeight = (int)(inputHeight * scale_y);

    int xOffset = (outputWidth - scaledWidth) / 2;
    int yOffset = (outputHeight - scaledHeight) / 2;

    int line_start = yOffset * outputWidth + xOffset;
    for (int y = 0; y < scaledHeight; ++y) {
        int srcY = (int)(y / scale_y);

        for (int x = 0; x < scaledWidth; ++x) {
            int srcX = (int)(x / scale_x);

            int doom_pixel = srcY * inputWidth + srcX;
            uint32_t index = input[(int)doom_pixel] * 3;
            uint32_t argb = 0xff000000 | (screen_palette[index] << 16) | (screen_palette[index + 1] << 8) |
                            screen_palette[index + 2];

            int oldX = srcX;
            int pixel = line_start + x;
            do {
                output[pixel++] = argb;
                srcX = (int)(x++ / scale_x);
            } while (srcX == oldX && x < scaledWidth);
            x--;
        }

        int oldY = srcY;
        do {
            srcY = (int)(y++ / scale_y);
            memcpy(&output[line_start + outputWidth], &output[line_start], scaledWidth * 4);
            line_start += outputWidth;
        } while (srcY == oldY && (y + 1) < scaledHeight);
        y--;
    }
}

int doom_entry(const char *wad_path)
{
    const char *args[] = {"doom"};
    int argc = 1;

    strcpy(wad_dir, wad_path);

    uint16_t refresh_rate = xbox_video_get_display_information()->refresh_rate;
    uint32_t output_width = xbox_video_get_display_information()->width;
    uint32_t output_height = xbox_video_get_display_information()->height;

    doom_set_print(dooom_printf);
    doom_set_malloc(dooom_malloc, dooom_free);
    doom_set_gettime(dooom_gettime);
    doom_set_file_io(dooom_open, dooom_close, dooom_read, dooom_write, dooom_seek, dooom_tell, dooom_eof);
    doom_set_getenv(dooom_getenv);

    printf_ts("[DOOM] Initializing...\n");

    doom_init(argc, (char **)args, 0);

    if (gamemode == indetermined) {
        printf_ts("[DOOM] Could not find suitable WAD files\n");
        while (1) {
            portYIELD();
        }
    }

    doom_initd = 1;

    doom_logic_mutex = xSemaphoreCreateMutex();
    doom_vblank_semaphore = xSemaphoreCreateBinary();
    xTaskCreate(doom_sound_task, "DoomSound", configMINIMAL_STACK_SIZE, &doom_logic_mutex, THREAD_PRIORITY_NORMAL,
                NULL);

    // Create our framebuffer output buffer
    uint32_t *draw_buffer = aligned_alloc(0x1000, output_width * output_height * 4);
    uint32_t *gpu_buffer1 = aligned_alloc(0x1000, output_width * output_height * 4);
    uint32_t *gpu_buffer2 = aligned_alloc(0x1000, output_width * output_height * 4);
    gpu_buffer1 = (uint32_t *)(((uint32_t)gpu_buffer1 + 0xFFF) & ~0xFFF);
    gpu_buffer2 = (uint32_t *)(((uint32_t)gpu_buffer2 + 0xFFF) & ~0xFFF);

    uint32_t *screen_buffer[2] = {XBOX_GET_WRITE_COMBINE_PTR(gpu_buffer1), XBOX_GET_WRITE_COMBINE_PTR(gpu_buffer2)};
    const uint32_t *p = draw_buffer;
    uint8_t backbuffer_index = 0;

    while (1) {
        uint32_t start_frame = xTaskGetTickCount();

        // Handle input
        xSemaphoreTake(doom_logic_mutex, portMAX_DELAY);
        doom_mouse_move(doom_rightx / 256, 0);
        doom_update();
        xSemaphoreGive(doom_logic_mutex);

        // Prepare our output buffer
        scale_doom((uint8_t *)doom_get_framebuffer(1), SCREENWIDTH, SCREENHEIGHT, draw_buffer, output_width,
                   output_height);

#if (1)
        // Draw FPS
        static int frames = 0;
        static uint32_t start_ticks_fps = 0;
        static uint32_t time = 0;
        if (frames++ == 9) {
            uint32_t end_ticks = xTaskGetTickCount();
            time = end_ticks - start_ticks_fps;
            start_ticks_fps = xTaskGetTickCount();
            frames = 0;
        }

        uint32_t x = 20;
        char fps_buffer[16];
        time = XBOX_MAX(time, 1);
        snprintf(fps_buffer, sizeof(fps_buffer), "FPS: %5d\n", 10000 / time);
        char *c = fps_buffer;
        while (*c) {
            display_write_char_ex(*c, x, 20, (void *)p);
            x += 8;
            c++;
        }
#endif

        // Blit draw buffer to gpu buffer
        void *gpu_backbuffer = screen_buffer[backbuffer_index ^= 1];
        memcpy(gpu_backbuffer, draw_buffer, output_width * output_height * 4);
        xbox_video_flush_cache();

        xbox_video_do_vblank_irq_one_shot(vblank_callback);
        xSemaphoreTake(doom_vblank_semaphore, (1000 / refresh_rate) + 1);

        xbox_video_set_option(XBOX_VIDEO_OPTION_FRAMEBUFFER, (uint32_t *)gpu_backbuffer);
    }

    free(draw_buffer);
    free(gpu_buffer1);
    free(gpu_buffer2);
    return 0;
}
