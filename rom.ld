STACK_SIZE = 32K;
ROM_VISOR_ENTRY_OFFSET = 0x00002000; /* Set by 2bxcodes.nasm */
RAM_ENTRY_OFFSET = 0x03D2C000; /* Squeeze it in just after a 640x480x4 framebuffer at 0x03C00000 */

MEMORY {
    ROM   (rx) : ORIGIN = 0xFFFC0000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x00000000, LENGTH = 64M
}

/* Sections */
SECTIONS {
    .rom_headers : {
        KEEP(*(.header00))
        . = 0x40;
        KEEP(*(.header40))
        . = 0x6C;
        KEEP(*(.header6C))
        . = 0x70;
        KEEP(*(.header70))
        . = 0x80;
        KEEP(*(.xcodes))
        . = 0x2000;
    } > ROM

    .visor_entry : {
        KEEP(*(.visor_entry))
    } > ROM

    .boot_code (RAM_ENTRY_OFFSET) : ALIGN(16) {
        KEEP(*(.boot_code));
        . = ALIGN(4);
    } > RAM AT> ROM

    .compressed : ALIGN(16) {
        KEEP(*(.compressed))
    } > ROM

    .text : ALIGN(16) {
        *(.text)
        *(.text.*)

        /* picolibc stuff */
        KEEP (*(.fini .fini.*))
        *(.got.plt)

        . = ALIGN(4);
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);

        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
        . = ALIGN(4);
    } > RAM

    .data : ALIGN(16) {
        *(.data)
        *(.data.*)
        . = ALIGN(4);
    } > RAM

    .rodata : ALIGN(16) {
        *(.rdata)
        *(.rdata.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
    } > RAM

    .tdata : ALIGN(16) {
        __tdata_start = .;
        *(.tdata)
        *(.tdata.*)
        *(.gnu.linkonce.td.*)
        __tdata_end = .;
        . = ALIGN(4);
    } > RAM

    .tbss (NOLOAD) : ALIGN(16) {
        __tbss_start = .;
        *(.tbss .tbss.* .gnu.linkonce.tb.*)
        *(.tcommon)
        __tbss_end = .;
    } > RAM

    /* The format of the picolibc TLS block is
     * Head: Stack protection (optional, .stack_chk)
     * Body: The combined .tdata and .tbss sections
     * Tail: The Thread Control Block (TCB)
    */
    __tls_align = MAX(ALIGNOF(.tdata), ALIGNOF(.tbss));
    .tls_space (NOLOAD) : ALIGN(__tls_align) {
        __tls_head_start = .;
        KEEP (*(.stack_chk .stack_chk.*))
        __tls_head_end = .;

        . = ALIGN(__tls_align);
        . = . + (ADDR(.tbss) + SIZEOF(.tbss) - ADDR(.tdata));

        . = ALIGN(__tls_align);
        __tls_tail_start = .;
        KEEP (*(.tls_tail_extra .tls_tail_extra.*))
        __tls_tail_end = .;
    } > RAM

    __tls_head_size  = ALIGN(__tls_head_end - __tls_head_start, __tls_align);
    __tls_space_size = ADDR(.tbss) + SIZEOF(.tbss) - ADDR(.tdata);
    __tls_tail_size  = __tls_tail_end - __tls_tail_start;
    __tbss_offset  = ADDR(.tbss) - ADDR(.tdata);
    __tbss_size    = SIZEOF(.tbss);
    __tls_size = __tls_head_size + ALIGN(__tls_space_size, __tls_align) + __tls_tail_size;
    __x86_tls_tcb_offset = ALIGN(__tls_head_size + __tls_space_size, __tls_align) + __tls_head_size;
    __tdata_source = LOADADDR(.tdata) - __tls_head_size;
    __tdata_size   = SIZEOF(.tdata) + __tls_head_size;

    .bss (NOLOAD) : ALIGN(16) {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        __bss_end = .;
        . = ALIGN(4);
    }

    .stack (NOLOAD) : ALIGN(16) {
        __stack_end = .;
        . += STACK_SIZE;
        . = ALIGN(16);
        __stack = .;
    }

    .heap (NOLOAD) : ALIGN(16) {
        __heap_start = .;
        . += LENGTH(RAM) - (__heap_start - ORIGIN(RAM));
        __heap_end = .;
    }

    /DISCARD/ : {
        *(.comment)
        *(.debug_info)
        *(.debug_abbrev)
        *(.debug_aranges)
        *(.debug_line)
        *(.debug_str)
        *(.debug_loclists)
        *(.debug_rnglists)
        *(.debug_line_str)
        *(.debug_str_offsets)
        *(.debug_addr)
        *(.debug_frame)
    }

    __user_text_vma = ADDR(.text);
    __user_data_vma = ADDR(.data);
    __user_rodata_vma = ADDR(.rodata);
    __user_tdata_vma = ADDR(.tdata);

    __user_text_size = SIZEOF(.text);
    __user_data_size = SIZEOF(.data);
    __user_rodata_size = SIZEOF(.rodata);
    __user_tdata_size = SIZEOF(.tdata);

    __boot_code_vma = ADDR(.boot_code);
    __boot_code_lma = LOADADDR(.boot_code);
    __boot_code_size = SIZEOF(.boot_code);

    /* The first 4 bytes is the CRC32 of the uncompressed data */
    /* Then it's the size of original uncompressed data in bytes */
    __uncompressed_data_crc = LOADADDR(.compressed);
    __uncompressed_data_size = LOADADDR(.compressed) + 4;
    __compressed_data_lma = LOADADDR(.compressed) + 8;

    __bss_dsize = SIZEOF(.bss) / 4;
}
