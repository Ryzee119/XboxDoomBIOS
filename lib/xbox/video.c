// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

uint8_t current_encoder_address = 0;
uint32_t current_output_mode_coding = 0;
display_information_t xbox_display_info;

// https://github.com/xemu-project/xemu/blob/8707d2aa2626063cb67b5ea20382584a0848dce7/hw/xbox/nv2a/nv2a.c#L87
typedef enum xbox_gpu_register
{
    PMC = 0x000000,      // Length = 0x1000
    PBUS = 0x001000,     // Length = 0x1000
    PFIFO = 0x002000,    // Length = 0x2000
    PRMA = 0x007000,     // Length = 0x1000
    PVIDEO = 0x008000,   // Length = 0x1000
    PTIMER = 0x009000,   // Length = 0x1000
    PCOUNTER = 0x00a000, // Length = 0x1000
    PVPE = 0x00b000,     // Length = 0x1000
    PTV = 0x00d000,      // Length = 0x1000
    PRMFB = 0x0a0000,    // Length = 0x20000
    PRMVIO = 0x0c0000,   // Length = 0x1000
    PFB = 0x100000,      // Length = 0x1000
    PSTRAPS = 0x101000,  // Length = 0x1000
    PGRAPH = 0x400000,   // Length = 0x2000
    PCRTC = 0x600000,    // Length = 0x1000
    PRMCIO = 0x601000,   // Length = 0x1000
    PRAMDAC = 0x680000,  // Length = 0x1000
    PRMDIO = 0x681000,   // Length = 0x1000
} xbox_gpu_register_t;

// See
// https://github.com/torvalds/linux/blob/3d5f968a177d468cd13568ef901c5be84d83d32b/drivers/gpu/drm/nouveau/dispnv04/nvreg.h#L368
// for some of these meanings
#define NV_PRAMDAC_GENERAL_CONTROL 0x00680600
#define NV_PRAMDAC_FP_VDISPLAY_END 0x00680800
#define NV_PRAMDAC_FP_VTOTAL       0x00680804
#define NV_PRAMDAC_FP_VCRTC        0x00680808
#define NV_PRAMDAC_FP_VSYNC_START  0x0068080c
#define NV_PRAMDAC_FP_VSYNC_END    0x00680810
#define NV_PRAMDAC_FP_VVALID_START 0x00680814
#define NV_PRAMDAC_FP_VVALID_END   0x00680818
#define NV_PRAMDAC_FP_HDISPLAY_END 0x00680820
#define NV_PRAMDAC_FP_HTOTAL       0x00680824
#define NV_PRAMDAC_FP_HCRTC        0x00680828
#define NV_PRAMDAC_FP_HSYNC_START  0x0068082c
#define NV_PRAMDAC_FP_HSYNC_END    0x00680830
#define NV_PRAMDAC_FP_HVALID_START 0x00680834
#define NV_PRAMDAC_FP_HVALID_END   0x00680838
#define NV_PRAMDAC_FP_MARGIN_COLOR 0x0068084c

// clang-format off
#define PRAMDAC_COUNT 26
static const uint32_t pramdac_registers_5838[][PRAMDAC_COUNT + 1] = {
    {0x0, 0x00680898, 0x0068089c, 0x006808c0, 0x006808c4, 0x0068084c, 0x00680630, 0x00680800, 0x00680804, 0x00680808, 0x0068080c, 0x00680810, 0x00680814, 0x00680818, 0x00680820, 0x00680824, 0x00680828, 0x0068082c, 0x00680830, 0x00680834, 0x00680838, 0x00680848, 0x00680680, 0x00680684, 0x00680688, 0x0068068c, 0x00680690},
    {0x1, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x000001df, 0x0000020c, 0x000001df, 0x000001e7, 0x000001e9, 0x00000000, 0x000001df, 0x0000027f, 0x0000030b, 0x00000257, 0x000002ab, 0x000002ad, 0x00000000, 0x0000027f, 0x10100111, 0x00000618, 0x00000001, 0x0000009e, 0x00000273, 0x00000000},
    {0x2, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x000001df, 0x0000020c, 0x000001df, 0x000001e7, 0x000001ed, 0x00000000, 0x000001df, 0x000002cf, 0x00000359, 0x000002a7, 0x000002f9, 0x000002ff, 0x00000000, 0x000002cf, 0x10100111, 0x000006b4, 0x00000001, 0x000000ae, 0x000002b2, 0x00000000},
    {0x3, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x000001df, 0x00000207, 0x000001df, 0x000001e1, 0x000001eb, 0x00000000, 0x000001df, 0x0000027f, 0x0000031f, 0x00000257, 0x000002bf, 0x000002d7, 0x00000000, 0x0000027f, 0x10100111, 0x000cb200, 0x00000271, 0x0000009f, 0x00000279, 0x00000000},
    {0x4, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x000001df, 0x00000207, 0x000001df, 0x000001e1, 0x000001eb, 0x00000000, 0x000001df, 0x000002cf, 0x00000383, 0x000002af, 0x000002f9, 0x0000032b, 0x00000000, 0x000002cf, 0x10100111, 0x000e4840, 0x00000271, 0x000000b3, 0x000002c8, 0x00000000},
    {0x6, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x0000023f, 0x00000270, 0x0000023f, 0x00000246, 0x0000024c, 0x00000000, 0x0000023f, 0x000002cf, 0x0000035f, 0x000002cc, 0x000002d5, 0x00000302, 0x00000000, 0x000002cf, 0x10100111, 0x000006c0, 0x00000001, 0x000000ae, 0x000002b2, 0x00000000},
    {0x7, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x000001df, 0x0000020c, 0x000001df, 0x000001e8, 0x000001ed, 0x00000000, 0x000001df, 0x0000027f, 0x0000030b, 0x00000257, 0x000002ab, 0x000002ad, 0x00000000, 0x0000027f, 0x10100111, 0x0000030c, 0x00000001, 0x0000009e, 0x00000273, 0x00000001},
    {0x8, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000002, 0x000001df, 0x0000020c, 0x000001df, 0x000001e8, 0x000001ed, 0x00000000, 0x000001df, 0x000002cf, 0x00000359, 0x000002a7, 0x000002f9, 0x000002ff, 0x00000000, 0x000002cf, 0x10100111, 0x0000035a, 0x00000001, 0x000000ae, 0x000002b2, 0x00000001},
    {0xB, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000003, 0x000002cf, 0x000002ed, 0x000002cf, 0x000002d4, 0x000002d9, 0x00000000, 0x000002cf, 0x000004ff, 0x00000671, 0x000004ff, 0x0000056d, 0x00000595, 0x00000000, 0x000004ff, 0x10100111, 0x00000672, 0x00000001, 0x00000157, 0x00000529, 0x00000001},
    {0xE, 0x10000000, 0x10000000, 0x00000000, 0x40801080, 0x00801080, 0x00000003, 0x00000437, 0x00000464, 0x0000043b, 0x0000043b, 0x00000445, 0x00000000, 0x00000437, 0x0000077f, 0x00000897, 0x000007ab, 0x000007dc, 0x00000804, 0x00000000, 0x0000077f, 0x10133111, 0x00000898, 0x00000001, 0x000001bf, 0x000006e2, 0x00000001},
};
#define PRAMDAC_OFFSETS &pramdac_registers_5838[0][1]
#define PRAMDAC_HEIGHT_OFFSET 0x00680818
#define PRAMDAC_WIDTH_OFFSET 0x00680820

#define PRMCIO_COUNT 34
static const uint8_t prmcio_registers_5838[][PRMCIO_COUNT + 1] = {
    {0x0, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x20, 0x25, 0x2d, 0x33, 0x39, 0x41},
    {0x1, 0x5d, 0x4f, 0x4f, 0x9c, 0x54, 0x35, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe9, 0x0e, 0xdf, 0x00, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x00, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11, 0xff, 0x00},
    {0x2, 0x66, 0x59, 0x59, 0x89, 0x5d, 0xbf, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe9, 0x0e, 0xdf, 0x00, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x00, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11, 0xff, 0x00},
    {0x3, 0x5f, 0x4f, 0x4f, 0x80, 0x55, 0xb9, 0x06, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x0e, 0xdf, 0x00, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x00, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11, 0xff, 0x00},
    {0x4, 0x6c, 0x59, 0x59, 0x86, 0x5f, 0xbf, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x0e, 0xdf, 0x00, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x00, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11, 0xff, 0x00},
    {0x6, 0x67, 0x59, 0x59, 0x69, 0x62, 0xdc, 0x6f, 0xf0, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x45, 0x08, 0x3f, 0x00, 0x00, 0x3f, 0x70, 0xe3, 0xff, 0x00, 0x3a, 0x05, 0x80, 0x10, 0x00, 0x11, 0xff, 0x00},
    {0x7, 0x59, 0x4f, 0x4f, 0x9d, 0x51, 0x39, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xe8, 0x0e, 0xdf, 0x40, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x38, 0x3a, 0x85, 0x80, 0x10, 0xe0, 0x11, 0xff, 0x00},
    {0x8, 0x63, 0x59, 0x59, 0x87, 0x5b, 0xa3, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00, 0xd0, 0x00, 0x00, 0x00, 0xe8, 0x0e, 0xdf, 0x80, 0x00, 0xdf, 0x0c, 0xe3, 0xff, 0x37, 0x3a, 0x85, 0x80, 0x10, 0xe0, 0x11, 0xff, 0x00},
    {0xA, 0xc8, 0x9f, 0x9f, 0x8c, 0xa7, 0x31, 0xec, 0xf0, 0x00, 0x60, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0xd4, 0x09, 0xcf, 0x80, 0x00, 0xcf, 0xed, 0xe3, 0xff, 0x5c, 0x38, 0x85, 0x80, 0x10, 0xc0, 0x11, 0xff, 0x00},
    {0xC, 0x04, 0xef, 0xef, 0x88, 0xf4, 0x3f, 0x2f, 0xf0, 0x00, 0x60, 0x00, 0x00, 0xb0, 0x00, 0x00, 0x00, 0x1d, 0x02, 0x1b, 0x00, 0x00, 0x1b, 0x30, 0xe3, 0xff, 0x00, 0x38, 0x85, 0x80, 0x00, 0xa1, 0x11, 0x10, 0x00},
};
#define PRMCIO_OFFSETS &prmcio_registers_5838[0][1]
// clang-format on

static inline void xbox_gpu_output32(xbox_gpu_register_t reg, uint32_t offset, uint32_t value)
{
    assert((offset & 3) == 0);
    volatile uint32_t *gpu_base = (volatile uint32_t *)PCI_GPU_MEMORY_REGISTER_BASE_0;
    gpu_base[(reg + offset) / 4] = value;
}

static inline uint32_t xbox_gpu_input32(xbox_gpu_register_t reg, uint32_t offset)
{
    assert((offset & 3) == 0);
    volatile uint32_t *gpu_base = (volatile uint32_t *)PCI_GPU_MEMORY_REGISTER_BASE_0;
    return gpu_base[(reg + offset) / 4];
}

static inline uint32_t xbox_gpu_input08(xbox_gpu_register_t reg, uint32_t offset)
{
    volatile uint8_t *gpu_base = (volatile uint8_t *)PCI_GPU_MEMORY_REGISTER_BASE_0;
    return gpu_base[(reg + offset)];
}

static inline void xbox_gpu_output08(xbox_gpu_register_t reg, uint32_t offset, uint8_t value)
{
    volatile uint8_t *gpu_base = (volatile uint8_t *)PCI_GPU_MEMORY_REGISTER_BASE_0;
    gpu_base[(reg + offset)] = value;
}

static void xbox_gpu_output_seq(uint8_t index, uint8_t value)
{
    xbox_gpu_output08(PRMVIO, 0x3C4, index);
    xbox_gpu_output08(PRMVIO, 0x3C5, value);
}

static void xbox_gpu_output_gra(uint8_t index, uint8_t value)
{
    xbox_gpu_output08(PRMVIO, 0x3CE, index);
    xbox_gpu_output08(PRMVIO, 0x3CF, value);
}

static void xbox_gpu_output_crtc(uint8_t index, uint8_t value)
{
    xbox_gpu_output08(PRMCIO, 0x3D4, index);
    xbox_gpu_output08(PRMCIO, 0x3D5, value);
}

#if (1)
static void xbox_gpu_output_clut(uint8_t regnum, uint8_t red, uint8_t green, uint8_t blue)
{
    xbox_gpu_output08(PRMDIO, 0x3C8, regnum);
    xbox_gpu_output08(PRMDIO, 0x3C9, red);
    xbox_gpu_output08(PRMDIO, 0x3C9, green);
    xbox_gpu_output08(PRMDIO, 0x3C9, blue);
}
#endif

static void xbox_gpu_set_output_enable(uint8_t enabled)
{
    if (enabled) {
        xbox_gpu_output_seq(0x01, 0x01);
    } else {
        xbox_gpu_output_seq(0x01, 0x21);
    }
}

// I basically wrote some homebrew to switch into every display mode and used xemu to log write to NV2A.
// Fortunately there isnt too many writes (~188). I used heuristics to work out patterns.
void xbox_video_init(uint32_t mode_coding, xbox_framebuffer_format_t format, void *frame_buffer)
{
    const VIDEO_MODE_SETTING *mode_settings = xbox_video_get_settings(mode_coding);

    const uint8_t pramdac_index = ((mode_coding & 0x00FF0000) >> 16);
    const uint8_t prmcio_index = (mode_coding & 0x0000FF00) >> 8;
    const uint32_t bpp = (format == ARGB8888) ? 4 : 2;
    const uint32_t width = mode_settings->width;
    const uint32_t pitch = (width * bpp) >> 3;
    uint32_t temp;

    // Work out which PRMCIO registers we should use
    const uint8_t *prmcio_offsets = PRMCIO_OFFSETS;
    const uint8_t *prmcio_values = NULL;
    for (uint8_t i = 0; i < PRMCIO_COUNT; i++) {
        if (prmcio_registers_5838[i][0] == prmcio_index) {
            prmcio_values = &prmcio_registers_5838[i][1];
            break;
        }
    }

    // Work out what PRAMDAC values we should use
    const uint32_t *pramdac_offsets = PRAMDAC_OFFSETS;
    const uint32_t *pramdac_values = NULL;
    for (uint8_t i = 0; i < PRAMDAC_COUNT; i++) {
        if (pramdac_registers_5838[i][0] == pramdac_index) {
            pramdac_values = &pramdac_registers_5838[i][1];
            break;
        }
    }
    assert(pramdac_values != NULL);
    assert(prmcio_values != NULL);

    //xbox_gpu_output32(PFB, 0x200, 0x03070103);
    //xbox_gpu_output32(PFB, 0x204, 0x11448000);

    // XPRINTF("[GPU] Setting up display mode %08X\n", mode_coding);

    current_output_mode_coding = mode_coding;

    if (current_encoder_address == 0) {
        current_encoder_address = xbox_encoder_detect();
    }

    // FIXME, can optimise this if we are already in the same mode (only changing xbox_framebuffer_format_t)
    // if(current_output_mode_coding == mode_coding)

    temp = 0;
    xbox_video_set_option(XBOX_VIDEO_OPTION_VIDEO_ENABLE, &temp);

    xbox_gpu_output_crtc(0x1F, 0x57);
    xbox_gpu_output_crtc(0x21, 0xFF);
    xbox_gpu_output_crtc(0x28, 0x00);
    xbox_gpu_output32(PRAMDAC, 0x880, 0x21121111);

    // Populate the display timing properties from the pramdac values
    for (uint8_t i = 0; i < PRAMDAC_COUNT; i++) {
        switch (pramdac_offsets[i]) {
            case NV_PRAMDAC_FP_VDISPLAY_END:
                xbox_display_info.vdisplay_end = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_VTOTAL:
                xbox_display_info.vtotal = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_VCRTC:
                xbox_display_info.vcrtc = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_VSYNC_START:
                xbox_display_info.vsync_start = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_VSYNC_END:
                xbox_display_info.vsync_end = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_VVALID_START:
                xbox_display_info.vvalid_start = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_VVALID_END:
                xbox_display_info.vvalid_end = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HDISPLAY_END:
                xbox_display_info.hdisplay_end = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HTOTAL:
                xbox_display_info.htotal = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HCRTC:
                xbox_display_info.hcrtc = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HSYNC_START:
                xbox_display_info.hsync_start = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HSYNC_END:
                xbox_display_info.hsync_end = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HVALID_START:
                xbox_display_info.hvalid_start = pramdac_values[i];
                break;
            case NV_PRAMDAC_FP_HVALID_END:
                xbox_display_info.hvalid_end = pramdac_values[i];
                break;
            default:
                break;
        }
    }

    xbox_display_info.bytes_per_pixel = bpp;
    xbox_display_info.frame_buffer = frame_buffer;
    xbox_display_info.refresh_rate = mode_settings->refresh;
    xbox_display_info.width = xbox_display_info.hvalid_end - xbox_display_info.hvalid_start + 1;
    xbox_display_info.height = xbox_display_info.vvalid_end - xbox_display_info.vvalid_start + 1;

    // XPRINTF xbox_display_info

    xbox_encoder_configure(mode_coding, &xbox_display_info);

    // 0x05 = widescreen aspect ratio, 0x04 is not?
    io_output_byte(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0xD6, (mode_coding & 0x10000000) ? 5 : 4);

    io_output_byte(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0xD8, 0x04);

    xbox_gpu_output32(PRAMDAC, 0x50C, 0x10020000);

    xbox_gpu_output08(PRMVIO, 0x3C3, 0x01);
    xbox_gpu_output08(PRMVIO, 0x3C2, 0xE3);

    // Not sure why RGB565 is the only one different
    if (format == RGB565) {
        xbox_gpu_output32(PRAMDAC, 0x600, 0x00101030);
    } else {
        xbox_gpu_output32(PRAMDAC, 0x600, 0x00100030);
    }

    // Write PRAMDAC registers
    for (uint8_t i = 0; i < PRAMDAC_COUNT; i++) {
        xbox_gpu_output32(PRAMDAC, pramdac_offsets[i] & 0xFFFF, pramdac_values[i]);
    }

    //? Seen only on these modes (SCART)
    if (mode_coding & 0x20000000) {
        xbox_gpu_output32(PRAMDAC, 0x630, 0);
        xbox_gpu_output32(PRAMDAC, 0x8C4, 0);
        xbox_gpu_output32(PRAMDAC, 0x84C, 0);
    }

    xbox_gpu_output_seq(0x00, 0x03);
    xbox_gpu_output_seq(0x01, 0x21);
    xbox_gpu_output_seq(0x02, 0x0F);
    xbox_gpu_output_seq(0x03, 0x00);
    xbox_gpu_output_seq(0x04, 0x06);

    xbox_gpu_output_gra(0x00, 0x00);
    xbox_gpu_output_gra(0x01, 0x00);
    xbox_gpu_output_gra(0x02, 0x00);
    xbox_gpu_output_gra(0x03, 0x00);
    xbox_gpu_output_gra(0x04, 0x00);
    xbox_gpu_output_gra(0x05, 0x40);
    xbox_gpu_output_gra(0x06, 0x05);
    xbox_gpu_output_gra(0x07, 0x0F);
    xbox_gpu_output_gra(0x08, 0xFF);

    const uint8_t vga_attr[] = {0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05,
                                0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A,
                                0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x10,
                                0x01, 0x11, 0x4A, 0x12, 0x0F, 0x13, 0x00, 0x14, 0x00, 0x20};
    for (uint8_t i = 0; i < XBOX_ARRAY_SIZE(vga_attr); i++) {
        xbox_gpu_output08(PRMCIO, 0x3C0, vga_attr[i]);
    }

    // Write CRTC to PRMCIO
    xbox_gpu_output_crtc(0x11, 0x00);
    for (uint8_t i = 0; i < PRMCIO_COUNT; i++) {

        // There a couple tweaks we need to make for pitch.
        // Fortunately this wasn't too hard to work out. When changing bpp only a couple registers change
        uint8_t prmcio_value = prmcio_values[i];
        if (prmcio_offsets[i] == 0x13) {
            prmcio_value = pitch & 0xFF;
        } else if (prmcio_offsets[i] == 0x19) {
            prmcio_value &= ~0xE0;
            prmcio_value |= ((pitch >> 3) & 0xE0);
        } else if (prmcio_offsets[i] == 0x25) {
            prmcio_value &= ~0x20;
            prmcio_value |= ((pitch >> 6) & 0x20);
        }
        xbox_gpu_output_crtc(prmcio_offsets[i], prmcio_value);
    }

    xbox_gpu_output32(PRAMDAC, 0x6A0, 0x01);
    xbox_gpu_output_crtc(0x28, ((bpp == 4) ? 0x83 : 0x82));

    for (int i = 0; i < 3; i++) {
        ;
        while ((xbox_gpu_input08(PRMCIO, 0x3DA) & 0x08) != 0x00)
            ;
        while ((xbox_gpu_input08(PRMCIO, 0x3DA) & 0x08) == 0x00)
            ;
    }

    xbox_gpu_output32(PRAMDAC, 0x880, (pramdac_index == 7) ? 0x21101101 : 0x21101100);

    xbox_timer_spin_wait(XBOX_TIMER_US_TO_TICKS(2));

    xbox_gpu_output32(PCRTC, 0x800, (uint32_t)frame_buffer);

    xbox_gpu_output08(PRMDIO, 0x3C8, 0);

    // Initialise the CLUT from 0 to 0xFF to 1:1 mapping
    xbox_gpu_output_clut(0, 0, 0, 0);
    for (int i = 1; i <= 0xFF; i++) {
        xbox_gpu_output08(PRMDIO, 0x3C9, i);
        xbox_gpu_output08(PRMDIO, 0x3C9, i);
        xbox_gpu_output08(PRMDIO, 0x3C9, i);
    }

    temp = 1;
    xbox_video_set_option(XBOX_VIDEO_OPTION_VIDEO_FLICKER_FILTER, &temp);
    temp = 1;
    xbox_video_set_option(XBOX_VIDEO_OPTION_VIDEO_ENABLE, &temp);

    xbox_interrupt_enable(XBOX_PIC_GPU_IRQ, 1);

#if (0)
    XPRINTF("Display Settings for mode %08x, bpp %d:\n", mode_coding, bpp);
    XPRINTF("xbox_display_info.vdisplay_end: %d %08x\n", xbox_display_info.vdisplay_end,
            xbox_display_info.vdisplay_end);
    XPRINTF("xbox_display_info.vtotal: %d %08x\n", xbox_display_info.vtotal, xbox_display_info.vtotal);
    XPRINTF("xbox_display_info.vcrtc: %d %08x\n", xbox_display_info.vcrtc, xbox_display_info.vcrtc);
    XPRINTF("xbox_display_info.vsync_start: %d %08x\n", xbox_display_info.vsync_start, xbox_display_info.vsync_start);
    XPRINTF("xbox_display_info.vsync_end: %d %08x\n", xbox_display_info.vsync_end, xbox_display_info.vsync_end);
    XPRINTF("xbox_display_info.vvalid_start: %d %08x\n", xbox_display_info.vvalid_start,
            xbox_display_info.vvalid_start);
    XPRINTF("xbox_display_info.vvalid_end: %d %08x\n", xbox_display_info.vvalid_end, xbox_display_info.vvalid_end);
    XPRINTF("xbox_display_info.hdisplay_end: %d %08x\n", xbox_display_info.hdisplay_end,
            xbox_display_info.hdisplay_end);
    XPRINTF("xbox_display_info.htotal: %d %08x\n", xbox_display_info.htotal, xbox_display_info.htotal);
    XPRINTF("xbox_display_info.hcrtc: %d %08x\n", xbox_display_info.hcrtc, xbox_display_info.hcrtc);
    XPRINTF("xbox_display_info.hsync_start: %d %08x\n", xbox_display_info.hsync_start, xbox_display_info.hsync_start);
    XPRINTF("xbox_display_info.hsync_end: %d %08x\n", xbox_display_info.hsync_end, xbox_display_info.hsync_end);
    XPRINTF("xbox_display_info.hvalid_start: %d %08x\n", xbox_display_info.hvalid_start,
            xbox_display_info.hvalid_start);
    XPRINTF("xbox_display_info.hvalid_end: %d %08x\n", xbox_display_info.hvalid_end, xbox_display_info.hvalid_end);
#endif
}

const display_information_t *xbox_video_get_display_information()
{
    return (const display_information_t *)&xbox_display_info;
}

static void *vblank_callback = NULL;
void gpu_handler()
{

    // Disable and reset VBLANK interrupt
    xbox_gpu_output32(PCRTC, 0x140, 0x00000000);
    xbox_gpu_output32(PCRTC, 0x100, 0x00000001);

    void *callback = vblank_callback;
    if (callback) {
        ((void (*)(void))callback)();
    }
}

void xbox_video_do_vblank_irq_one_shot(void (*callback)(void))
{
    vblank_callback = callback;

    // Enable hardware interrupts
    xbox_gpu_output32(PMC, 0x140, 0x00000001);

    // Reset pending VBLANK interrupt
    xbox_gpu_output32(PCRTC, 0x100, 0x00000001);

    // Enable VBLANK interrupt
    xbox_gpu_output32(PCRTC, 0x140, 0x00000001);
}

uint8_t xbox_video_set_option(xbox_video_option_t option, uint32_t *parameter)
{
    uint32_t temp;
    if (parameter == NULL) {
        return XBOX_VIDEO_RETURN_ERROR;
    }

    if (current_encoder_address == 0) {
        current_encoder_address = xbox_encoder_detect();
    }

    switch (option) {
        case XBOX_VIDEO_OPTION_VIDEO_ENABLE:
            // XPRINTF("[VIDEO] Turning %s\n", (*parameter) ? "on" : "off");
            //  Switch it in gpu and at hardware level
            if (*parameter) {
                xbox_gpu_set_output_enable(1);
                xbox_gpu_output32(PRAMDAC, 0x6A0, 0x01);
                io_output_byte(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0xD3, 0x04);
            } else {
                xbox_gpu_set_output_enable(0);
                xbox_gpu_output32(PRAMDAC, 0x6A0, 0x00);
                io_output_byte(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0xD3, 0x05);
            }

            // Switch it at the encoder
            switch (current_encoder_address) {
                case XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR:
                    if (*parameter) {
                        uint8_t output = (current_output_mode_coding >> 24) & 0x0F;
                        smbus_output_dword(current_encoder_address, 0x04, output);
                    } else {
                        smbus_input_dword(current_encoder_address, 0x04, &temp);
                        smbus_output_dword(current_encoder_address, 0x04, temp | 0x00000000F);
                        smbus_input_dword(current_encoder_address, 0x00, &temp);
                        smbus_output_dword(current_encoder_address, 0x00, temp & 0xFFFFFFFD);
                    }
                    break;
                case XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT:
                    if (*parameter) {
                        smbus_output_byte(current_encoder_address, 0xBA, 0x3F);
                    }
                    break;
                case XBOX_SMBUS_ADDRESS_ENCODER_FOCUS:
                    if (*parameter) {
                        smbus_output_byte(current_encoder_address, FOCUS_CTRL_PWR_MGNT_16, 0x00);
                    } else {
                        smbus_output_byte(current_encoder_address, FOCUS_CTRL_PWR_MGNT_16, 0x0F);
                    }
                    break;
                default:
                    return XBOX_VIDEO_RETURN_ERROR;
            }
            break;
        case XBOX_VIDEO_OPTION_VIDEO_FLICKER_FILTER:
            switch (current_encoder_address) {
                case XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR:
                    break;
                case XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT: {
                    uint8_t conex_c8 = 0x80;
                    uint8_t conex_34 = 0x00;
                    smbus_input_byte(current_encoder_address, 0xC8, &conex_c8);
                    if (*parameter == 0) {
                        conex_c8 |= 0x40;
                        smbus_output_byte(current_encoder_address, 0xC8, conex_c8);
                    } else {
                        conex_c8 &= 0x80;
                        if (*parameter >= 1 && *parameter <= 3) {
                            conex_c8 |= (*parameter << 0) | (*parameter << 3);
                        } else if (*parameter == 5) {
                            conex_34 = 0x80;
                        }
                        smbus_output_byte(current_encoder_address, 0xC8, conex_c8);
                        smbus_output_byte(current_encoder_address, 0x34, conex_34);
                    }
                } break;
                case XBOX_SMBUS_ADDRESS_ENCODER_FOCUS:
                    if (*parameter == 0) {
                        *parameter = 0;
                    } else if (*parameter <= 2) {
                        *parameter = 13;
                    } else {
                        *parameter = 16;
                    }
                    smbus_output_byte(current_encoder_address, FOCUS_SDTVI_FLK_16, (uint16_t)(*parameter + 0xFF));
                    smbus_output_byte(current_encoder_address, FOCUS_SDTVI_FLK_16, (uint16_t)(*parameter + 0x00));

                    break;
                default:
                    return XBOX_VIDEO_RETURN_ERROR;
            }
            break;
        case XBOX_VIDEO_OPTION_VIDEO_SOFTEN_FILTER:
            switch (current_encoder_address) {
                case XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR:
                    break;
                case XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT: {
                    uint8_t conex_96;
                    smbus_input_byte(current_encoder_address, 0x96, &conex_96);
                    conex_96 &= 0x0F;
                    if (*parameter) {
                        conex_96 |= 0x10;
                    }
                    smbus_output_byte(current_encoder_address, 0x96, conex_96);
                } break;
                    break;
                case XBOX_SMBUS_ADDRESS_ENCODER_FOCUS:
                default:
                    return XBOX_VIDEO_RETURN_ERROR;
            }
            break;
        case XBOX_VIDEO_OPTION_FRAMEBUFFER:
            xbox_gpu_output32(PCRTC, 0x800, (uint32_t)parameter);
            xbox_display_info.frame_buffer = (void *)parameter;
            break;
        default:
            return XBOX_VIDEO_RETURN_ERROR;

            break;
    }
    return XBOX_VIDEO_RETURN_SUCCESS;
}

static VIDEO_MODE_SETTING video_modes[] = {
    {0x44030307, 640, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_STANDARD},   // 640x480 PAL 50Hz
    {0x44040408, 720, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_STANDARD},   // 720x480 PAL 50Hz
    {0x0401010B, 640, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_STANDARD},   // 640x480 PAL 60Hz
    {0x0402020C, 720, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_STANDARD},   // 720x480 PAL 60Hz
    {0x04010101, 640, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_STANDARD}, // 640x480 NTSCM 60Hz
    {0x04020202, 720, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_STANDARD}, // 720x480 NTSCM 60Hz
    {0x04010103, 640, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_STANDARD}, // 640x480 NTSCJ 60Hz
    {0x04020204, 720, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_STANDARD}, // 720x480 NTSCJ 60Hz

    {0x60030307, 640, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SCART},   // 640x480 PAL 50Hz RGB
    {0x60040408, 720, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SCART},   // 720x480 PAL 50Hz RGB
    {0x2001010B, 640, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SCART},   // 640x480 PAL 60Hz RGB
    {0x2002020C, 720, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SCART},   // 720x480 PAL 60Hz RGB
    {0x20010101, 640, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_SCART}, // 640x480 NTSCM 60Hz RGB
    {0x20020202, 720, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_SCART}, // 720x480 NTSCM 60Hz RGB
    {0x20010103, 640, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_SCART}, // 640x480 NTSCJ 60Hz RGB
    {0x20020204, 720, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_SCART}, // 720x480 NTSCJ 60Hz RGB

    {0x48030314, 640, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_HDTV},     // 640x480i PAL 50Hz
    {0x48040415, 720, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_HDTV},     // 720x480i PAL 50Hz
    {0x08010119, 640, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_HDTV},     // 640x480i PAL 60Hz
    {0x0802021a, 720, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_HDTV},     // 720x480i PAL 60Hz
    {0x0801010d, 640, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_HDTV},   // 640x480i NTSCM 60Hz
    {0x0802020e, 720, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_HDTV},   // 720x480i NTSCM 60Hz
    {0x88070701, 640, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_HDTV},   // 640x480p NTSCM 60Hz
    {0x88080801, 720, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_HDTV},   // 720x480p NTSCM 60Hz
    {0x880B0A02, 1280, 720, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_HDTV},  // 1280x720p NTSCM 60Hz
    {0x880E0C03, 1920, 1080, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_HDTV}, // 1920x1080i NTSCM 60Hz
    {0x0801010d, 640, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_HDTV},   // 640x480i NTSCJ 60Hz
    {0x0802020e, 720, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_HDTV},   // 720x480i NTSCJ 60Hz
    {0x88070701, 640, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_HDTV},   // 640x480p NTSCJ 60Hz
    {0x88080801, 720, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_HDTV},   // 720x480p NTSCJ 60Hz
    {0x880B0A02, 1280, 720, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_HDTV},  // 1280x720p NTSCJ 60Hz
    {0x880E0C03, 1920, 1080, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_HDTV}, // 1920x1080i NTSCJ 60Hz

    {0x44030307, 640, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SVIDEO},   // 640x480 PAL 50Hz
    {0x44040408, 720, 480, 50, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SVIDEO},   // 720x480 PAL 50Hz
    {0x0401010B, 640, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SVIDEO},   // 640x480 PAL 60Hz
    {0x0402020C, 720, 480, 60, XBOX_VIDEO_REGION_PAL, XBOX_AV_PACK_SVIDEO},   // 720x480 PAL 60Hz
    {0x04010101, 640, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_SVIDEO}, // 640x480 NTSCM 60Hz
    {0x04020202, 720, 480, 60, XBOX_VIDEO_REGION_NTSCM, XBOX_AV_PACK_SVIDEO}, // 720x480 NTSCM 60Hz
    {0x04010103, 640, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_SVIDEO}, // 640x480 NTSCJ 60Hz
    {0x04020204, 720, 480, 60, XBOX_VIDEO_REGION_NTSCJ, XBOX_AV_PACK_SVIDEO}, // 720x480 NTSCJ 60Hz
};

const VIDEO_MODE_SETTING *xbox_video_get_settings(uint32_t mode_coding)
{
    for (int i = 0; i < XBOX_ARRAY_SIZE(video_modes); i++) {
        if (video_modes[i].mode == mode_coding) {
            return &video_modes[i];
        }
    }
    return NULL;
}

uint32_t xbox_video_get_suitable_mode_coding(uint32_t width, uint32_t height)
{
    xbox_av_pack_t avpack;
    xbox_eeprom_t *eeprom = xbox_eeprom_get();
    xbox_video_region_t video_region = eeprom->factory_settings.video_standard & XBOX_EEPROM_VIDEO_STANDARD_MASK;
    uint8_t refresh = 60;

    if (xbox_smc_get_avpack(&avpack) < 0) {
        return 0;
    }

    if (video_region == XBOX_VIDEO_REGION_PAL &&
        !(eeprom->user_settings.video_settings & XBOX_EEPROM_VIDEO_SETTINGS_60HZ)) {
        refresh = 50;
    }

    for (uint32_t i = 0; i < XBOX_ARRAY_SIZE(video_modes); i++) {
        if (video_modes[i].width == width && video_modes[i].height == height && video_modes[i].refresh == refresh &&
            video_modes[i].avpack == avpack && video_modes[i].video_region == video_region) {
            return video_modes[i].mode;
        }
    }

    return 0;
}

void apply_all_video_modes(void *fb)
{
    for (int i = 0; i < XBOX_ARRAY_SIZE(video_modes); i++) {
        XPRINTF("\r\n%d Mode: %08x, Width: %d, Height: %d, Refresh: %d, BPP: 32\r", i, video_modes[i].mode,
                video_modes[i].width, video_modes[i].height, video_modes[i].refresh);
        xbox_video_init(video_modes[i].mode, ARGB8888, fb);
    }
}

// Perhaps last cache line when copying to write combine buffer is not flushed?
void xbox_video_flush_cache()
{
    __asm__ volatile("sfence");
    __asm__ volatile("wbinvd ");
}
