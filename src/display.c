#include "main.h"

static const uint8_t MARGIN = 20;
static uint32_t cursor_x = MARGIN;
static uint32_t cursor_y = MARGIN;

void apply_all_video_modes(void *fb);
void display_init()
{
    int width = 640;
    int height = 480;
    int bpp = 4;

    uint32_t mode_coding = xbox_video_get_suitable_mode_coding(width, height);

    // Invalid input, fallback to 640x480 to hopefully show something
    if (mode_coding == 0) {
        // This should always be valid
        printf_ts("[DISPLAY] Failed to find a suitable mode, trying 640x480\n");
        width = 640;
        height = 480;
        mode_coding = xbox_video_get_suitable_mode_coding(width, height);
        if (mode_coding == 0) {
            mode_coding = 0x04010101; // Just pick one to hopefully show something
            printf_ts("[DISPLAY] Failed to find a suitable mode, forcing 640x480\n");
        }
    }

    uint8_t *fb = aligned_alloc(0x1000, width * height * bpp);
    fb = XBOX_GET_WRITE_COMBINE_PTR(fb);
    memset(fb, 0x00, width * height * bpp);
    xbox_video_flush_cache();
    printf_ts("[DISPLAY] Using mode %08X\n", mode_coding);
    xbox_video_init(mode_coding, (bpp == 2) ? RGB565 : ARGB8888, fb);
}

void display_clear()
{
    const display_information_t *display = xbox_video_get_display_information();
    if (display->frame_buffer == NULL) {
        return;
    }
    void *fb = display->frame_buffer;
    memset(fb, 0x00, display->width * display->height * display->bytes_per_pixel);
    xbox_video_flush_cache();
    cursor_x = MARGIN;
    cursor_y = MARGIN;
}

void display_get_cursor(uint32_t *x, uint32_t *y)
{
    if (x) {
        *x = cursor_x;
    }
    if (y) {
        *y = cursor_y;
    }
}

void display_set_cursor(uint32_t x, uint32_t y)
{
    const display_information_t *display = xbox_video_get_display_information();
    if (!display->frame_buffer) {
        return;
    }

    const uint32_t minx = MARGIN;
    const uint32_t maxx = display->width - MARGIN - UNSCII_FONT_WIDTH;
    const uint32_t miny = MARGIN;
    const uint32_t maxy = display->height - MARGIN - UNSCII_FONT_HEIGHT;

    x = XBOX_CLAMP(minx, x, maxx);
    y = XBOX_CLAMP(miny, y, maxy);

    cursor_x = x;
    cursor_y = y;
}

void display_write_char_ex(const char c, uint16_t x, uint16_t y, void *frame_buffer) {
    const display_information_t *display = xbox_video_get_display_information();

    uint16_t *fb16 = (uint16_t *)frame_buffer;
    uint32_t *fb32 = (uint32_t *)frame_buffer;

    // To save space we purged the first 32 'non-ascii' characters from the font, so ignore anything below 0x20
    // then offset c by -0x20 to get the correct index into the font
    if (c > 0x7F || c < 0x20) {
        return;
    }
    const uint8_t *glyph = unscii_16 + ((c - 0x20) * ((UNSCII_FONT_WIDTH + 7) / 8) * UNSCII_FONT_HEIGHT);
    for (int h = 0; h < UNSCII_FONT_HEIGHT; h++) {
        uint8_t mask = 0x80;
        for (int w = 0; w < UNSCII_FONT_WIDTH; w++) {
            const uint32_t pixel = (y + h) * display->width + x + w;
            if (display->bytes_per_pixel == 2) {
                fb16[pixel] = (*glyph & (mask >>= 1)) ? 0xFFFF : 0x0000;
            } else {
                fb32[pixel] = (*glyph & (mask >>= 1)) ? 0xFFFFFFFF : 0xFF000000;
            }
        }
        glyph++; // Next line of glyph
    }
}

void display_write_char(const char c)
{
    const display_information_t *display = xbox_video_get_display_information();
    if (!display->frame_buffer) {
        return;
    }

    void *fb = display->frame_buffer;
    uint16_t *fb16 = (uint16_t *)fb;
    uint32_t *fb32 = (uint32_t *)fb;

    if (c == '\n') {
        cursor_x = MARGIN;
        cursor_y += UNSCII_FONT_HEIGHT;
    } else if (c == '\r') {
        cursor_x = MARGIN;
    } else {
        display_write_char_ex(c, cursor_x, cursor_y, fb);
        cursor_x += UNSCII_FONT_WIDTH;
    }

    // Wrap text
    if (cursor_x + UNSCII_FONT_WIDTH >= (display->width - MARGIN)) {
        cursor_x = MARGIN;
        cursor_y += UNSCII_FONT_HEIGHT;
    }

    // New page
    if (cursor_y + UNSCII_FONT_HEIGHT >= (display->height - MARGIN)) {
#if (0)
        // Clear screen and start again
        display_clear();
        return;
#else
        // Scroll up one row
        const uint32_t pixel_total = display->width * display->height * display->bytes_per_pixel;
        const uint32_t pixel_per_row = display->width * UNSCII_FONT_HEIGHT * display->bytes_per_pixel;
        memcpy(fb, fb + pixel_per_row, pixel_total - pixel_per_row);
        memset(fb + pixel_total - pixel_per_row, 0x00, pixel_per_row);
        cursor_y -= UNSCII_FONT_HEIGHT;
        cursor_x = MARGIN;
#endif
    }
}
