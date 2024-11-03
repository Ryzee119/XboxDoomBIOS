#ifndef VIDEO_H
#define VIDEO_H

#include "video.h"
#include "smc.h"
#include "xbox.h"

typedef struct display_information
{
    void *frame_buffer;
    uint16_t bytes_per_pixel;
    uint16_t refresh_rate;
    uint16_t width;
    uint16_t height;
    uint16_t vdisplay_end;
    uint16_t vtotal;
    uint16_t vcrtc;
    uint16_t vsync_start;
    uint16_t vsync_end;
    uint16_t vvalid_start;
    uint16_t vvalid_end;
    uint16_t hdisplay_end;
    uint16_t htotal;
    uint16_t hcrtc;
    uint16_t hsync_start;
    uint16_t hsync_end;
    uint16_t hvalid_start;
    uint16_t hvalid_end;
} display_information_t;

enum
{
    XBOX_VIDEO_OPTION_VIDEO_ENABLE,
    XBOX_VIDEO_OPTION_VIDEO_FLICKER_FILTER,
    XBOX_VIDEO_OPTION_VIDEO_SOFTEN_FILTER,
    XBOX_VIDEO_OPTION_FRAMEBUFFER,
};
typedef uint8_t xbox_video_option_t;

enum
{
    ARGB1555,
    RGB565,
    ARGB8888,
};
typedef uint8_t xbox_framebuffer_format_t;

#define XBOX_VIDEO_RETURN_SUCCESS 0
#define XBOX_VIDEO_RETURN_ERROR   -1

typedef enum xbox_video_region {
    XBOX_VIDEO_REGION_NTSCM = 0x00000100,
    XBOX_VIDEO_REGION_NTSCJ = 0x00000200,
    XBOX_VIDEO_REGION_PAL   = 0x00000300,
} xbox_video_region_t;

typedef struct _VIDEO_MODE_SETTING
{
    uint32_t mode;
    uint16_t width;
    uint16_t height;
    uint8_t refresh;
    xbox_video_region_t video_region;
    xbox_av_pack_t avpack;
} VIDEO_MODE_SETTING;

#define XBOX_VIDEO_MAKE_COLOR_RGB565(r, g, b)       (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))
#define XBOX_VIDEO_MAKE_COLOUR_ARGB8888(a, r, g, b) (((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))

void xbox_video_do_vblank_irq_one_shot(void (*callback)(void));
uint32_t xbox_video_get_suitable_mode_coding(uint32_t width, uint32_t height);
void xbox_video_init(uint32_t mode_coding, xbox_framebuffer_format_t format, void *frame_buffer);
uint8_t xbox_video_set_option(xbox_video_option_t option, uint32_t *parameter);
void xbox_video_flush_cache();
const display_information_t *xbox_video_get_display_information();
const VIDEO_MODE_SETTING *xbox_video_get_settings(uint32_t mode_coding);

#endif
