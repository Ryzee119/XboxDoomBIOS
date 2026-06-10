#include "../main.h"

uint32_t load_elf_payload(const char *filepath);

#define SEABIOS_COREBOOT_TABLE_ADDR 0x00000800 // Needs to be betwen 0 and 0x1000
#define MEM_PHYSICAL_END            0x04000000 // 64MB: Top of physical RAM
#define MEM_FRAMEBUFFER_BASE        0x03C00000 // 60MB: 4MB reserved for video framebuffer

// https://github.com/coreboot/seabios/tree/master/src/fw/coreboot.c

#define CB_SIGNATURE 0x4f49424C // "LBIO"

struct cb_header
{
    uint32_t signature;
    uint32_t header_bytes;
    uint32_t header_checksum;
    uint32_t table_bytes;
    uint32_t table_checksum;
    uint32_t table_entries;
};

struct cb_memory_range
{
    uint64_t start;
    uint64_t size;
    uint32_t type;
};

#define CB_MEM_TABLE 16

struct cb_memory
{
    uint32_t tag;
    uint32_t size;
    struct cb_memory_range map[4];
};

#define CB_TAG_MEMORY      0x01
#define CB_TAG_FRAMEBUFFER 0x0012

struct cb_framebuffer
{
    uint32_t tag;
    uint32_t size;
    uint64_t physical_address;
    uint32_t x_resolution;
    uint32_t y_resolution;
    uint32_t bytes_per_line;
    uint8_t bits_per_pixel;
    uint8_t red_mask_pos;
    uint8_t red_mask_size;
    uint8_t green_mask_pos;
    uint8_t green_mask_size;
    uint8_t blue_mask_pos;
    uint8_t blue_mask_size;
    uint8_t reserved_mask_pos;
    uint8_t reserved_mask_size;
};

// Seabios uses a simple checksum algorithm for both the header and the table
static uint32_t compute_ip_checksum(void *addr, uint32_t length)
{
    uint16_t *ptr = (uint16_t *)addr;
    uint32_t sum = 0;

    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }

    if (length > 0) {
        sum += *(uint8_t *)ptr;
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return (uint32_t)(~sum & 0xffff);
}

static void build_coreboot_table(void *target_address)
{
    // 1. Pointers
    struct cb_header *header = (struct cb_header *)target_address;
    struct cb_memory *mem_tag = (struct cb_memory *)((uint8_t *)header + sizeof(struct cb_header));
    struct cb_framebuffer *fb_tag = (struct cb_framebuffer *)((uint8_t *)mem_tag + sizeof(struct cb_memory));

    // 2. Populate Memory Tag (CRITICAL for SeaBIOS to run correctly)
    mem_tag->tag = CB_TAG_MEMORY; // 0x01
    mem_tag->size = sizeof(struct cb_memory);

    // Range 0: 0 to 640KB (Usable)
    mem_tag->map[0].start = 0;
    mem_tag->map[0].size = 640 * 1024;
    mem_tag->map[0].type = 1;

    // Range 1: 640KB to 1MB (Reserved)
    mem_tag->map[1].start = 0x000A0000;
    mem_tag->map[1].size = 0x00060000;
    mem_tag->map[1].type = 2;

    // Range 2: 1MB to 60MB (Usable - Main OS RAM)
    mem_tag->map[2].start = 0x00100000;
    mem_tag->map[2].size = MEM_FRAMEBUFFER_BASE - 0x00100000;
    mem_tag->map[2].type = 1;

    // Range 3: 60MB to 64MB (Reserved for Xbox Video)
    mem_tag->map[3].start = MEM_FRAMEBUFFER_BASE;
    mem_tag->map[3].size = MEM_PHYSICAL_END - MEM_FRAMEBUFFER_BASE;
    mem_tag->map[3].type = 2;

    // 3. Populate Framebuffer Tag
    fb_tag->tag = CB_TAG_FRAMEBUFFER; // 0x12
    fb_tag->size = sizeof(struct cb_framebuffer);

    // Use the WR pointer for performance.
    fb_tag->physical_address = 0xF0000000 | MEM_FRAMEBUFFER_BASE;

    fb_tag->x_resolution = 640;
    fb_tag->y_resolution = 480;
    fb_tag->bits_per_pixel = 32;
    fb_tag->bytes_per_line = 640 * 4;

    fb_tag->red_mask_pos = 16;
    fb_tag->red_mask_size = 8;
    fb_tag->green_mask_pos = 8;
    fb_tag->green_mask_size = 8;
    fb_tag->blue_mask_pos = 0;
    fb_tag->blue_mask_size = 8;
    fb_tag->reserved_mask_pos = 24;
    fb_tag->reserved_mask_size = 8;

    // 4. Update Header
    header->signature = CB_SIGNATURE; // "LBIO"
    header->header_bytes = sizeof(struct cb_header);
    header->header_checksum = 0;

    header->table_bytes = mem_tag->size + fb_tag->size;
    header->table_entries = 2;

    // 5. Checksums
    header->table_checksum = compute_ip_checksum(mem_tag, header->table_bytes);
    header->header_checksum = compute_ip_checksum(header, header->header_bytes);
}

void boot_seabios(void)
{
    build_coreboot_table((void *)SEABIOS_COREBOOT_TABLE_ADDR);

    // Move framebuffer as we setup in the E820 map
    xbox_video_set_option(XBOX_VIDEO_OPTION_FRAMEBUFFER, (void *)MEM_FRAMEBUFFER_BASE);
    display_clear();

    FILE *vga_file = fopen("D:/vgabios.bin", "rb");
    if (vga_file) {
        fread((void *)0x00200000, 1, 65536, vga_file);
        fclose(vga_file);
        printf("Loaded VGA BIOS to 0xC0000\n");
    } else {
        printf("CRITICAL ERROR: Could not find D:/vgabios.bin\n");
    }

    uint32_t entry_point = load_elf_payload("D:/bios.bin.elf");
    printf("Seabios ELF entry point: 0x%08X\n", entry_point);

    __asm__ volatile(
        // 1. Disable Interrupts
        "cli \n\t"

        // 2. Disable Paging
        // Read CR0, clear the Paging bit (bit 31), and write it back.
        "mov %%cr0, %%eax \n\t"
        "and $0x7FFFFFFF, %%eax \n\t"
        "mov %%eax, %%cr0 \n\t"

        // 3. Set up Flat Data Segments
        "mov $0x18, %%ax \n\t"
        "mov %%ax, %%ds \n\t"
        "mov %%ax, %%es \n\t"
        "mov %%ax, %%fs \n\t"
        "mov %%ax, %%gs \n\t"

        // We also align the stack segment to the flat data segment to prevent
        // faults if SeaBIOS pushes to the stack before setting up its own.
        "mov %%ax, %%ss \n\t"

        // 4. The Absolute Jump
        // Jump directly to the address held in the input register (%0)
        "jmp *%0 \n\t"

        :                  /* No output operands */
        : "r"(entry_point) /* Input operand: map entry_point to a register */
        : "eax"            /* Clobber list: tell compiler we touched EAX */
    );

    printf("Error: Seabios returned control to the loader, which should never happen!\n");

    // Fallback halt just in case the compiler complains about noreturn
    while (1)
        ;

    // CPU should never reach this point
}
