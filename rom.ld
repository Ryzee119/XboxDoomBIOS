STACK_SIZE = 32K;
ROM_VISOR_ENTRY_OFFSET = 0x00001000; /* Set by 2bxcodes.nasm */
RAM_ENTRY_OFFSET = 0x00100000;
/*FFF40000*/
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
        . = 0x1000;
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
        *(.tbss.*)

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

    __user_text_size = SIZEOF(.text);
    __user_data_size = SIZEOF(.data);
    __user_rodata_size = SIZEOF(.rodata);

    __boot_code_vma = ADDR(.boot_code);
    __boot_code_lma = LOADADDR(.boot_code);
    __boot_code_size = SIZEOF(.boot_code);

    /* first 4 bytes are the size of original uncompressed data */
    __uncompressed_data_crc = LOADADDR(.compressed);
    __uncompressed_data_size = LOADADDR(.compressed) + 4;
    __compressed_data_lma = LOADADDR(.compressed) + 8;

    __bss_dsize = SIZEOF(.bss) / 4;
}
