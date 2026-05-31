#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../main.h"
#include "linux_boot_param.h"

// https://www.kernel.org/doc/Documentation/x86/boot.txt
// Memory map
#define MEM_PHYSICAL_END     0x04000000 // 64MB: Top of physical RAM
#define MEM_FRAMEBUFFER_BASE 0x03C00000 // 60MB: 4MB reserved for video framebuffer

// Linux Boot Protocol (High Memory)
#define LINUX_PM_ENTRY 0x00100000 // 32-bit Protected Mode payload (bzImage)

// Linux Boot Protocol (Low Memory)
#define LINUX_BOOT_END       0x000A0000 // End of low memory reserved for bootloader and zero page
#define LINUX_BOOT_GDT       0x0009FF00 // Temporary GDT for transition
#define LINUX_CMDLINE_BASE   0x0009FE00 // Kernel command line arguments
#define LINUX_SETUP_HEAP_END 0x0009FCFF // Heap end pointer for setup
#define LINUX_SETUP_CODE     0x00090200 // Real mode setup code entry
#define LINUX_BOOT_PARAMS    0x00090000 // Zero Page / boot_params struct
#define LINUX_BOOT_STACK     0x00090000 // Stack for real mode setup code (counting down from start of boot_params)

#define STR(x)  #x
#define XSTR(x) STR(x)

static int parse_linux_cfg(const char *linuxcfg_path, char *kernel, size_t kernel_sz, char *initrd,
                               size_t initrd_sz, char *append, size_t append_sz);

void boot_linux(const char *linuxcfg_path)
{
    char kernel[256];
    char initrd[256];
    char cmdline[512];
    kernel[0] = '\0';
    initrd[0] = '\0';
    cmdline[0] = '\0';

    if (parse_linux_cfg(linuxcfg_path, kernel, sizeof(kernel), initrd, sizeof(initrd), cmdline, sizeof(cmdline)) ==
        0) {

        printf("Kernel Path: %s\n", kernel);

        if (initrd[0] != '\0') {
            printf("Initrd Path: %s\n", initrd);
        } else {
            printf("Initrd Path: (None Provided)\n");
        }

        printf("Command Args: %s\n", cmdline);
    } else {
        printf("Failed to open or read %s\n", linuxcfg_path);
        return;
    }

    struct boot_params *bp = (struct boot_params *)LINUX_BOOT_PARAMS;
    strncpy((char *)LINUX_CMDLINE_BASE, cmdline, 255);

    memset(bp, 0, sizeof(struct boot_params));

    // Read in the bzImage header and kernel payload into memory
    FILE *f_bz = fopen(kernel, "rb");
    if (f_bz) {
        fseek(f_bz, 0, SEEK_END);
        const size_t bz_size = ftell(f_bz);
        fseek(f_bz, 0, SEEK_SET);

        // First read the setup_header from the bzImage to verify magic and get kernel entry point
        fseek(f_bz, offsetof(struct boot_params, hdr.setup_sects), SEEK_SET);
        fread(&bp->hdr, 1, sizeof(struct setup_header), f_bz);

        // Verify boot_flag magic
        if (bp->hdr.boot_flag != 0xAA55) {
            printf("Warning: Invalid bzImage boot flag! 0x%04X\n", bp->hdr.boot_flag);
        }

        // Verify magic signature "HdrS" (Present on protocol 2.00+)
        if (bp->hdr.header != 0x53726448) {
            printf("Warning: Invalid bzImage signature! 0x%08X. Assuming old boot protocol\n", bp->hdr.header);
        }

        // The 32-bit (non-real-mode) kernel starts at offset (setup_sects + 1) * 512
        // For backwards compatibility, if the setup_sects field contains 0, the real value is 4.
        // Seek to it, then read it into RAM at the required code32_start address specified in the header
        const long kernel_offset = ((bp->hdr.setup_sects ? bp->hdr.setup_sects : 4) + 1) * 512;
        fseek(f_bz, kernel_offset, SEEK_SET);

        if (fread((void *)bp->hdr.code32_start, 1, bz_size - kernel_offset, f_bz) != bz_size - kernel_offset) {
            printf("Warning: Payload size mismatch. Expected %zu bytes, read %ld bytes\n",
                   bz_size - (size_t)kernel_offset, ftell(f_bz) - kernel_offset);
        }
        fclose(f_bz);
    } else {
        printf("Error: Failed to open bzImage %s. We have no kernel to boot.\n", kernel);
        return;
    }

    /* 4. Load initrd if provided */
    if (initrd[0] != '\0') {
        FILE *f_initrd = fopen(initrd, "rb");
        if (f_initrd) {
            fseek(f_initrd, 0, SEEK_END);
            size_t initrd_size = ftell(f_initrd);
            fseek(f_initrd, 0, SEEK_SET);

            // Place initrd at the end of physical memory just before the framebuffer, aligned to 4KB
            void *initrd_dest = (void *)((MEM_FRAMEBUFFER_BASE - initrd_size) & ~0xFFF);
            fread(initrd_dest, 1, initrd_size, f_initrd);
            fclose(f_initrd);

            bp->hdr.ramdisk_image = (uint32_t)initrd_dest;
            bp->hdr.ramdisk_size = initrd_size;
            bp->hdr.initrd_addr_max = MEM_PHYSICAL_END;
        } else {
            printf("Warning: Failed to open initrd %s\n", initrd);
        }
    } else {
        printf("No initrd specified, continuing without it.\n");
    }

    // Header
    bp->hdr.vid_mode = 0x312;      // 640x480x16M Colors
    bp->hdr.type_of_loader = 0xFF; // Unknown/Custom Bootloader ID
    bp->hdr.loadflags = bp->hdr.loadflags | CAN_USE_HEAP;
    bp->hdr.heap_end_ptr = LINUX_SETUP_HEAP_END - LINUX_SETUP_CODE + 1;
    bp->hdr.cmd_line_ptr = (uint32_t)LINUX_CMDLINE_BASE;
    bp->hdr.cmdline_size = 255;

    // Screen Info https://github.com/torvalds/linux/blob/master/include/linux/screen_info.h
    const display_information_t *disp_info = xbox_video_get_display_information();
    bp->screen.orig_video_isVGA = VIDEO_TYPE_VLFB;
    bp->screen.orig_video_cols = disp_info->width / 8;
    bp->screen.orig_video_lines = disp_info->height / 16;
    bp->screen.orig_video_points = 16;
    bp->screen.lfb_depth = disp_info->bytes_per_pixel * 8;
    bp->screen.lfb_width = disp_info->width;
    bp->screen.lfb_height = disp_info->height;
    bp->screen.lfb_base = (uint32_t)XBOX_GET_WRITE_COMBINE_PTR(MEM_FRAMEBUFFER_BASE);
    bp->screen.lfb_size = (MEM_PHYSICAL_END - MEM_FRAMEBUFFER_BASE) / 0x10000;
    bp->screen.lfb_linelength = disp_info->width * disp_info->bytes_per_pixel;
    bp->screen.pages = 1;

    // RGB565 for 16bpp, ARGB8888 for 32bpp
    if (bp->screen.lfb_depth == 16) {
        bp->screen.orig_video_mode = 0x11; // 640x480x2
        bp->screen.red_size = 5;
        bp->screen.red_pos = 11;
        bp->screen.green_size = 6;
        bp->screen.green_pos = 5;
        bp->screen.blue_size = 5;
        bp->screen.blue_pos = 0;
        bp->screen.rsvd_size = 0;
        bp->screen.rsvd_pos = 0;
    } else {
        bp->screen.orig_video_mode = 0x12; // 640x480x4
        bp->screen.red_size = 8;
        bp->screen.red_pos = 16;
        bp->screen.green_size = 8;
        bp->screen.green_pos = 8;
        bp->screen.blue_size = 8;
        bp->screen.blue_pos = 0;
        bp->screen.rsvd_size = 8;
        bp->screen.rsvd_pos = 24;
    }

    // Legacy command line
    bp->screen.cl_magic = OLD_CL_MAGIC;
    bp->screen.cl_offset = LINUX_CMDLINE_BASE - LINUX_BOOT_PARAMS;

    // Legacy RAM flags
    const uint32_t ext_mem_kb = (MEM_PHYSICAL_END - LINUX_PM_ENTRY) / 1024;
    bp->screen.ext_mem_k = (ext_mem_kb > 0xFFFF) ? 0xFFFF : ext_mem_kb;

    // E820 Memory Map
    bp->e820_entries = 0;
    {
        // Lower RAM (0 to 640KB)
        bp->e820_table[bp->e820_entries++] = (struct e820_entry){0x00000000, LINUX_BOOT_END, E820_TYPE_RAM};

        // Reserved Area
        bp->e820_table[bp->e820_entries++] =
            (struct e820_entry){LINUX_BOOT_END, LINUX_PM_ENTRY - LINUX_BOOT_END, E820_TYPE_RESERVED};

        // High Usable RAM
        bp->e820_table[bp->e820_entries++] =
            (struct e820_entry){bp->hdr.code32_start, MEM_FRAMEBUFFER_BASE - bp->hdr.code32_start, E820_TYPE_RAM};

        // Reserved: Framebuffer
        bp->e820_table[bp->e820_entries++] =
            (struct e820_entry){MEM_FRAMEBUFFER_BASE, MEM_PHYSICAL_END - MEM_FRAMEBUFFER_BASE, E820_TYPE_RESERVED};
    }

    printf("Final setup and jumping to Linux kernel...\n");

    // Disable OHCI
    *((volatile uint32_t *)(PCI_USB0_MEMORY_REGISTER_BASE_0 + 0x00)) = 0x00000000; // OHCI

    // Construct a standard 4-entry flat GDT expected by Linux
    volatile uint64_t *low_gdt = (uint64_t *)LINUX_BOOT_GDT;
    low_gdt[0] = 0x0000000000000000ULL; // 0x00: Unused
    low_gdt[1] = 0x0000000000000000ULL; // 0x08: Unused
    low_gdt[2] = 0x00CF9A000000FFFFULL; // 0x10: 32-bit Code, Flat 4GB, Exec/Read
    low_gdt[3] = 0x00CF92000000FFFFULL; // 0x18: 32-bit Data, Flat 4GB, Read/Write
    struct
    {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed)) gdtr = {.limit = (4 * 8) - 1, .base = (uint32_t)low_gdt};

    // Disable interrupts
    __asm__ __volatile__("cli" ::: "memory");

    // Move framebuffer as we setup in the E820 map
    xbox_video_set_option(XBOX_VIDEO_OPTION_FRAMEBUFFER, (void *)MEM_FRAMEBUFFER_BASE);

    // It's safe to do this because
    display_clear();
    const char *msg = "Loading Linux kernel...";
    while (*msg) {
        display_write_char(*msg++);
    }

    __asm__ __volatile__(
        // Flush the cache
        "wbinvd\n\t"

        // Disable Paging (just in case, should already be disabled in our environment)
        "movl %%cr0, %%eax\n\t"
        "andl $0x7FFFFFFF, %%eax\n\t"
        "movl %%eax, %%cr0\n\t"

        // Flush TLB
        "xorl %%eax, %%eax\n\t"
        "movl %%eax, %%cr3\n\t"

        // Load new GDT
        "lgdt %0\n\t"

        // Reload Data Segment (0x18) for linux entry expectations.
        "movw $0x0018, %%ax\n\t"
        "movw %%ax, %%ss\n\t"
        "movw %%ax, %%ds\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%ax, %%fs\n\t"
        "movw %%ax, %%gs\n\t"

        // Set up a safe stack in low memory (just below our boot_params) counting down.
        "movl $" XSTR(LINUX_BOOT_STACK) ", %%esp\n\t"

        // Setup the far jump to reload CS with the new GDT and the kernel entry point.
        "pushl $0x10\n\t"
        "pushl %2\n\t"

        // Clear general purpose registers (as required by Linux)
        "xorl %%eax, %%eax\n\t"
        "xorl %%ebx, %%ebx\n\t"
        "xorl %%ecx, %%ecx\n\t"
        "xorl %%edx, %%edx\n\t"
        "xorl %%ebp, %%ebp\n\t"
        "xorl %%edi, %%edi\n\t"

        // Jump to the kernel
        "lret\n\t"
        :                           /* No outputs */
        : "m"(gdtr),                // %0: The GDT descriptor for our new GDT
          "S"((uint32_t)bp),        // %1: "S" forces GCC to initialize %esi with `bp`
          "r"(bp->hdr.code32_start) // %2: GCC picks a GPR. It's pushed to the stack before clearing.
        : "eax", "memory");         // Declare %eax as clobbered so GCC doesn't map %1 to it.

    // CPU should never reach this point
}

static int parse_linux_cfg(const char *linuxcfg_path, char *kernel, size_t kernel_sz, char *initrd,
                               size_t initrd_sz, char *append, size_t append_sz)
{
    FILE *fp = fopen(linuxcfg_path, "r");
    if (!fp) {
        printf("Error: Could not open Linux config file %s\n", linuxcfg_path);
        return -1;
    }

    // Initialize output buffers to empty strings
    if (kernel && kernel_sz > 0) {
        kernel[0] = '\0';
    }
    if (initrd && initrd_sz > 0) {
        initrd[0] = '\0';
    }
    if (append && append_sz > 0) {
        append[0] = '\0';
    }

    char line[1024];

    while (fgets(line, sizeof(line), fp)) {
        char *p = line;

        // Skip leading whitespace
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }

        // Skip empty lines or comments
        if (*p == '\0' || *p == '#') {
            continue;
        }

        char *val = NULL;
        char *dest = NULL;
        size_t dest_max = 0;

        // Identify the key and set the appropriate destination buffer
        if (strncmp(p, "kernel", 6) == 0 && isspace((unsigned char)p[6])) {
            val = p + 7;
            dest = kernel;
            dest_max = kernel_sz;
        } else if (strncmp(p, "initrd", 6) == 0 && isspace((unsigned char)p[6])) {
            val = p + 7;
            dest = initrd;
            dest_max = initrd_sz;
        } else if (strncmp(p, "append", 6) == 0 && isspace((unsigned char)p[6])) {
            val = p + 7;
            dest = append;
            dest_max = append_sz;
        }

        // If a matching key was found and a valid buffer was provided
        if (dest && dest_max > 0) {
            // Skip whitespace between the key and the value
            while (*val && isspace((unsigned char)*val)) {
                val++;
            }

            // Strip trailing whitespace and newlines from the value
            char *end = val + strlen(val) - 1;
            while (end >= val && isspace((unsigned char)*end)) {
                *end = '\0';
                end--;
            }

            // First put the leading path from filepath (e.g. "D:/") into the destination buffer, then append the value
            // from the config
            if (strncmp(p, "append", 6) == 0) {
                // For append, we don't want to prepend the path, just copy the value directly
                strncpy(dest, val, dest_max - 1);
                dest[dest_max - 1] = '\0';
                continue;
            }

            char *last_slash = strrchr(linuxcfg_path, '/');
            if (last_slash) {
                size_t prefix_len = last_slash - linuxcfg_path + 1;
                if (prefix_len < dest_max) {
                    strncpy(dest, linuxcfg_path, prefix_len);
                    dest[prefix_len] = '\0';
                } else {
                    // If the prefix is too long, just start with an empty string
                    dest[0] = '\0';
                }
            }

            strncat(dest, val, dest_max - strlen(dest) - 1);
        }
    }

    fclose(fp);
    return 0;
}
