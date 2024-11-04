; SPDX-License-Identifier: MIT
; Copyright (c) 2024 Ryzee119

extern __user_text_vma
extern __user_text_size

extern __user_data_vma
extern __user_data_size

extern __user_rodata_vma
extern __user_rodata_size

extern __boot_code_vma
extern __boot_code_lma
extern __boot_code_size

extern __bss_dsize
extern __bss_start
extern __stack

extern __compressed_data_lma;
extern __uncompressed_data_size;

extern LZ4_decompress_fast
extern boot_pic_challenge_response
extern boot

%macro WR_MSR 3 ; addr, hi, lo
    mov ecx, %1
    mov edx, %2
    mov eax, %3
    wrmsr
%endmacro

; MCPX Magic Values
section .header00
    dd 0xff000009
    dd 0xff000008
    dd 0x2b16d065
    dd 0x3346322d
    dd 0x01010101
    dd 0x08080808
    dd 0x00000801

%ifdef MCPXREVD5
    dd 0xc8b4588a   ; MCPX 1.6 > D5
    dd 0x00100000
    dd 0x00050aa7
    dd 0xf0000000
%else
    dd 0xc8fc7c8a   ; MCPX =< D5
    dd 0x44290213
    dd 0x90004998
    dd 0x00000000
%endif

    dd 0xffffffff
    dd 0xffffffff

section .header40
    dd 0x00000000

section .header6C
    dd 0x00000107

; MCPX Config Area 
section .header70 
    dd 0x0000000f
    dd 0x40004400

%ifdef MCPXREVD5
    dd 0x16ce0090
    dd 0x00000dc0
%else
    dd 0x16ce0090
    dd 0x00000000
%endif

section .xcodes
    %include '2bxcodes.nasm'

; This is our entry point after visor roll over
section .visor_entry
  
    ; Clear Intel Interrupts in Processor Register
    mov ecx, 0x1b
    xor eax, eax
    xor edx, edx
    wrmsr

    ; Clear registers
    xor eax, eax
    xor edx, edx
    xor ecx, ecx

    ; Disable cache
    mov eax, cr0
    or eax, (0x60000000)
    mov cr0, eax
    wbinvd

    ; Clear CR3
    mov eax, eax
    mov cr3, eax

    ; Disable MTRRs
    WR_MSR 0x2ff, 0, 0

    ; 0 +128M WB (RAM) (0x00000000 + 128MB)
    ; 128 MB region starting at address 0x00000000 to be cached as Write-Back (WB) memory
    WR_MSR 0x200, 0x00000000, 0x00000006 ; Base
    WR_MSR 0x201, 0x0000000f, 0xf8000800 ; Mask

    ; 0xf0000000 +128M WC (VRAM) (0xF0000000 + 128MB)
    ; 128 MB region starting at address 0xf0000000 to be cached as Write-Combining (WC) memory
    WR_MSR 0x202, 0x00000000, 0xf0000001 ; Base
    WR_MSR 0x203, 0x0000000f, 0xf8000800 ; Mask

    ; 0xfff00000 +1M WP (Flash)
    ; 1 MB region starting at address 0xfff00000 to be cached as Write-Protected (WP) memory
    WR_MSR 0x204, 0x00000000, 0xfff00000 ; Base
    WR_MSR 0x205, 0x0000000f, 0xfff00800 ; Mask

    ; Clear other MTRRs
    WR_MSR 0x206, 0x00000000, 0x00000000
    WR_MSR 0x207, 0x00000000, 0x00000000
    WR_MSR 0x208, 0x00000000, 0x00000000
    WR_MSR 0x209, 0x00000000, 0x00000000
    WR_MSR 0x20A, 0x00000000, 0x00000000
    WR_MSR 0x20B, 0x00000000, 0x00000000
    WR_MSR 0x20C, 0x00000000, 0x00000000
    WR_MSR 0x20D, 0x00000000, 0x00000000
    WR_MSR 0x20E, 0x00000000, 0x00000000
    WR_MSR 0x20F, 0x00000000, 0x00000000

    ; Enable MTRRs
    WR_MSR 0x2ff, 0x00000000, 0x00000800

    mov eax, eax
    mov cr3, eax
    wbinvd

    ; Enable Caching
    mov eax, cr0
    mov ebx, eax
    and eax, 0x9FFFFFFF
    mov cr0, eax

    ; Copy initial boot code to RAM
    cld
    mov     edi, __boot_code_vma
    mov     esi, __boot_code_lma
    mov     ecx, __boot_code_size
    shr     ecx, 2
    rep     movsd

    ; Set the stack pointer
    mov esp, __stack

    ; Clear out .bss
    xor eax, eax
    mov ecx, __bss_dsize
    mov edi, __bss_start
    rep stosd

    ; Jump to initial boot code
    jmp boot_entry;

section .boot_code
align 16
gdt_table:
    dq 0x0000000000000000  ; Dummy
    dq 0x00CF9B000000FFFF  ; 0x0008 code32
    dq 0x00CF93000000FFFF  ; 0x0010 data32

gdt_desc:
    dw 24-1                ; Limit (size of gdt_table - 1)
    dd gdt_table           ; Base address of gdt_table
    dw 0x00;

idt_desc:
    dw 256*8-1             ; Limit (size of idt_table - 1)
    dd idt_table           ; Base address of idt_table
    dw 0x00;

boot_entry:
    ; Ensure interrupts are disabled to update gdt/idt
    cli

    ; Set the Global Descriptor Table and Interrupt Descriptor Table
    lgdt [gdt_desc]
    lidt [idt_desc]

    xor eax, eax
    lldt ax

    ; As we changed gdt while in protected mode, we need to reload the code segment selector
    ; this is done by doing a long jump with the code32 offset
    jmp 0x0008:reload_segment_selectors

align 16
reload_segment_selectors:
    ; Set the data segment registers now
    mov ax, 0x0010
    mov ss, ax
    mov ds, ax
    mov es, ax

    ; Clear fs and gs
    xor eax, eax
    mov fs, eax
    mov gs, eax

    ; Clear out .bss
    xor eax, eax
    mov ecx, __bss_dsize
    mov edi, __bss_start
    rep stosd

    ; ?
    ;outb(0x61, 0x08);
    mov     al, 0x8
    mov     dx, 0x61
    out     dx, al

    ; Need to do this quickly after boot, so we do it before we decompress and copy user code 
    ; into RAM
    call boot_pic_challenge_response

    ; Decompress user code directly into RAM 0x3C00000 is at ~60MB its plenty of space we know is free
    push dword [__uncompressed_data_size]
    push dword 0x3C00000
    push dword __compressed_data_lma
    call LZ4_decompress_fast
    add esp, 16

    ; Copy text section
    mov     edi, __user_text_vma
    mov     esi, 0x3C00000 ; 0x3C00000 + __user_text_size
    mov     ecx, __user_text_size
    shr     ecx, 2
    rep     movsd

    ; Copy data section
    mov     edi, __user_data_vma
    mov     esi, 0x3C00000
    add     esi, __user_text_size
    mov     ecx, __user_data_size
    shr     ecx, 2
    rep     movsd

    ; Copy rodata section
    mov     edi, __user_rodata_vma
    mov     esi, 0x3C00000 ; 0x3C00000 + __user_text_size + __user_data_size
    add     esi, __user_text_size
    add     esi, __user_data_size
    mov     ecx, __user_rodata_size
    shr     ecx, 2
    rep     movsd

    ; Enable Floating Point
    mov eax, cr4
    or  eax, (1<<9 | 1<<10)
    mov cr4, eax
    clts
    fninit

    ; Jump to C code
    jmp boot


; Reserved space for compressed data filled during compilation
section .compressed
    dq 0

; Reserved space for idt table
section .bss
idt_table:
    resq 256
