#include <stdint.h>

/* ram_size flags */
#define RAMDISK_IMAGE_START_MASK 0x07FF
#define RAMDISK_PROMPT_FLAG      0x8000
#define RAMDISK_LOAD_FLAG        0x4000

/* loadflags */
#define LOADED_HIGH   (1 << 0)
#define KASLR_FLAG    (1 << 1)
#define QUIET_FLAG    (1 << 5)
#define KEEP_SEGMENTS (1 << 6)
#define CAN_USE_HEAP  (1 << 7)

// https://github.com/torvalds/linux/tree/master/include/uapi/linux/screen_info.h
#define VIDEO_TYPE_VLFB 0x23 /* VESA VGA in graphic mode	*/

// https://github.com/torvalds/linux/trea/master/arch/x86/include/asm/setup.h
#define OLD_CL_MAGIC 0xA33F

struct screen_info
{
    uint8_t orig_x;             /* 0x00 */
    uint8_t orig_y;             /* 0x01 */
    uint16_t ext_mem_k;         /* 0x02 */
    uint16_t orig_video_page;   /* 0x04 */
    uint8_t orig_video_mode;    /* 0x06 */
    uint8_t orig_video_cols;    /* 0x07 */
    uint8_t flags;              /* 0x08 */
    uint8_t unused2;            /* 0x09 */
    uint16_t orig_video_ega_bx; /* 0x0a */
    uint16_t unused3;           /* 0x0c */
    uint8_t orig_video_lines;   /* 0x0e */
    uint8_t orig_video_isVGA;   /* 0x0f */
    uint16_t orig_video_points; /* 0x10 */

    // VESA fields (valid if orig_video_isVGA != 0)
    uint16_t lfb_width;       /* 0x12 */
    uint16_t lfb_height;      /* 0x14 */
    uint16_t lfb_depth;       /* 0x16 */
    uint32_t lfb_base;        /* 0x18 */
    uint32_t lfb_size;        /* 0x1c */
    uint16_t cl_magic;        /* 0x20 */
    uint16_t cl_offset;       /* 0x22 */
    uint16_t lfb_linelength;  /* 0x24 */
    uint8_t red_size;         /* 0x26 */
    uint8_t red_pos;          /* 0x27 */
    uint8_t green_size;       /* 0x28 */
    uint8_t green_pos;        /* 0x29 */
    uint8_t blue_size;        /* 0x2a */
    uint8_t blue_pos;         /* 0x2b */
    uint8_t rsvd_size;        /* 0x2c */
    uint8_t rsvd_pos;         /* 0x2d */
    uint16_t vesapm_seg;      /* 0x2e */
    uint16_t vesapm_off;      /* 0x30 */
    uint16_t pages;           /* 0x32 */
    uint16_t vesa_attributes; /* 0x34 */
    uint32_t capabilities;    /* 0x36 */
    uint32_t ext_lfb_base;    /* 0x3a */
    uint8_t _reserved[2];     /* 0x3e */
} __attribute__((packed));

/* Standard E820 memory map entry */
enum e820_type
{
    E820_TYPE_RAM = 1,
    E820_TYPE_RESERVED = 2,
    E820_TYPE_ACPI = 3,
    E820_TYPE_NVS = 4,
    E820_TYPE_UNUSABLE = 5,
    E820_TYPE_PMEM = 7,
    E820_TYPE_PRAM = 12,
    E820_TYPE_SOFT_RESERVED = 0xefffffff,
};

struct e820_entry
{
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));

/* The setup header (starts at offset 0x01f1 in boot_params) */
struct setup_header
{
    uint8_t setup_sects;            /* 0x01f1 */
    uint16_t root_flags;            /* 0x01f2 */
    uint32_t syssize;               /* 0x01f4 */
    uint16_t ram_size;              /* 0x01f8 */
    uint16_t vid_mode;              /* 0x01fa */
    uint16_t root_dev;              /* 0x01fc */
    uint16_t boot_flag;             /* 0x01fe */
    uint16_t jump;                  /* 0x0200 */
    uint32_t header;                /* 0x0202 (Magic signature "HdrS") */
    uint16_t version;               /* 0x0206 (Boot protocol version) */
    uint32_t realmode_swtch;        /* 0x0208 */
    uint16_t start_sys_seg;         /* 0x020c */
    uint16_t kernel_version;        /* 0x020e */
    uint8_t type_of_loader;         /* 0x0210 */
    uint8_t loadflags;              /* 0x0211 */
    uint16_t setup_move_size;       /* 0x0212 */
    uint32_t code32_start;          /* 0x0214 */
    uint32_t ramdisk_image;         /* 0x0218 */
    uint32_t ramdisk_size;          /* 0x021c */
    uint32_t bootsect_kludge;       /* 0x0220 */
    uint16_t heap_end_ptr;          /* 0x0224 */
    uint8_t ext_loader_ver;         /* 0x0226 */
    uint8_t ext_loader_type;        /* 0x0227 */
    uint32_t cmd_line_ptr;          /* 0x0228 */
    uint32_t initrd_addr_max;       /* 0x022c */
    uint32_t kernel_alignment;      /* 0x0230 */
    uint8_t relocatable_kernel;     /* 0x0234 */
    uint8_t min_alignment;          /* 0x0235 */
    uint16_t xloadflags;            /* 0x0236 */
    uint32_t cmdline_size;          /* 0x0238 */
    uint32_t hardware_subarch;      /* 0x023c */
    uint64_t hardware_subarch_data; /* 0x0240 */
    uint32_t payload_offset;        /* 0x0248 */
    uint32_t payload_length;        /* 0x024c */
    uint64_t setup_data;            /* 0x0250 */
    uint64_t pref_address;          /* 0x0258 */
    uint32_t init_size;             /* 0x0260 */
    uint32_t handover_offset;       /* 0x0264 */
    uint32_t kernel_info_offset;    /* 0x0268 */
} __attribute__((packed));

/* The complete 4KB Zero Page / boot_params */
struct boot_params
{
    struct screen_info screen; /* 0x000 - 0x03f */

    /* Pad from end of screen_info (0x40) to e820_entries (0x1e8) */
    uint8_t _pad0[0x1e8 - 0x040];

    uint8_t e820_entries; /* 0x1e8 */
    uint8_t _pad1[8];     /* 0x1e9 - 0x1f0 */

    struct setup_header hdr; /* 0x1f1 - 0x26b */

    /* Pad from end of setup_header to E820 table at 0x2d0 */
    uint8_t _pad2[0x2d0 - 0x1f1 - sizeof(struct setup_header)];

    struct e820_entry e820_table[128]; /* 0x2d0 - 0xcd0 (128 entries * 20 bytes) */

    /* Pad out the rest to exactly 4096 bytes (4KB) */
    uint8_t _pad3[0x1000 - 0xcd0];
} __attribute__((packed));