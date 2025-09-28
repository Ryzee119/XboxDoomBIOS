; SPDX-License-Identifier: GPL-2.0
; Copyright (c) https://github.com/XboxDev/cromwell
;
; https://github.com/Ernegien/xdecode
; xdecode -i cromwell.bin -c comments.conf

BITS 32

; ---------------------------------------------------------------------------
; Opcode definitions
; ---------------------------------------------------------------------------
op_mem_read     EQU 0x02
op_mem_write    EQU 0x03
op_pci_write    EQU 0x04
op_pci_read     EQU 0x05
op_andor        EQU 0x06
op_chain        EQU 0x07
op_jne          EQU 0x08
op_jmp          EQU 0x09
op_andorebp     EQU 0x10
op_io_write     EQU 0x11
op_io_read      EQU 0x12
op_exit         EQU 0xEE
op_unused1      EQU 0xF5
op_unused2      EQU 0x80

; ---------------------------------------------------------------------------
; Generic macros
; ---------------------------------------------------------------------------
%macro xc_opcode 3
    db %1
    dd %2, %3
%endmacro

%macro xc_mem_read 1
    xc_opcode op_mem_read, %1, 0
%endmacro

%macro xc_mem_write 2
    xc_opcode op_mem_write, %1, %2
%endmacro

%macro xc_pci_write 2
    xc_opcode op_pci_write, %1, %2
%endmacro

%macro xc_pci_read 1
    xc_opcode op_pci_read, %1, 0
%endmacro

%macro xc_andor 2
    xc_opcode op_andor, %1, %2
%endmacro

%macro xc_chain 2
    xc_opcode op_chain, %1, %2
%endmacro

%macro xc_jne 2
    xc_opcode op_jne, %1, %2 - $ - 9 + 1
%endmacro

%macro xc_jmp 1
    xc_opcode op_jmp, 0, %1 - $ - 9 + 1
%endmacro

%macro xc_andorebp 2
    xc_opcode op_andorebp, %1, %2
%endmacro

%macro xc_io_write 2
    xc_opcode op_io_write, %1, %2
%endmacro

%macro xc_io_read 1
    xc_opcode op_io_read, %1, 0
%endmacro

%macro xc_exit 0
    xc_opcode op_exit, 0, 0
%endmacro

; ---------------------------------------------------------------------------
; SMBus macros
; ---------------------------------------------------------------------------

; %1 = device address (e.g., 0xE0)
; %2 = success jump label
%macro smbus_poke 2
    xc_andorebp 0, %1                   ; Put the address in scratch
    xc_andor 0xFFFFFFFF, 0x000000001    ; OR a 1 to indicate a READ
    xc_chain op_io_write, 0x0000C004    ; Set SMBUS_ADDRESS
    xc_io_write 0x0000C000, 0x000000FF  ; Clear SMBUS_STATUS
    xc_io_write 0x0000C002, 0x00000009  ; SMBUS_CONTROL: zero + start

%%smbus_busy:
    xc_io_read 0x0000C000               ; Read SMBUS_STATUS
    xc_andor 0x00000008, 0x00000000     ; Pull out SMBUS_STATUS_BUSY
    xc_jne 0x00000008, %%done
    xc_jmp %%smbus_busy

%%done:
    xc_io_read 0x0000C000               ; Read SMBUS_STATUS again
    xc_jne 0x00000010, %%smbus_error
    xc_jmp %2                           ; Success jump

%%smbus_error:                          ; On error we fall through
%endmacro

; %1 = device address (0x00 = use previous)
; %2 = command
; %3 = byte to write
; No error checking (retail style)
%macro smbus_write_byte 3
    %if %1 != 0
        xc_io_write 0x0000C004, %1      ; SMBUS_ADDRESS
    %endif
    xc_io_write 0x0000C008, %2          ; SMBUS_COMMAND
    xc_io_write 0x0000C006, %3          ; SMBUS_DATA
    xc_io_write 0x0000C000, 0x000000FF  ; Clear status
    xc_io_write 0x0000C002, 0x0000000A  ; TYPE_BYTE + START
%%smbus_loop_until_complete:
    xc_io_read 0x0000C000               ; SMBUS_STATUS
    xc_jne 0x00000010, %%smbus_loop_until_complete
%endmacro

; ---------------------------------------------------------------------------
; Serial output macro
; ---------------------------------------------------------------------------
%macro xc_puts 1+
    %assign __i 0
    %strlen __len %1
    %rep __len
        %substr __ch %1 __i,1
        xc_io_write 0x03F8, __ch
        %assign __i __i+1
    %endrep
    xc_io_write 0x03F8, 0x0A
%endmacro

xc_opcode op_unused1, 0x00081000, 1
xc_opcode op_unused2, 0x00000114, 0x228

; Seems safe to write both X2 and X3 variants instead of determining the revision at runtime
xc_pci_write 0x80000884, 0x00008001 ; LPC bridge MCPX X2. Set IO bar to 0x8000
xc_pci_write 0x80000810, 0x00008001 ; LPC bridge MCPX X3. Set IO bar to 0x8000
xc_pci_write 0x80000804, 0x00000003 ; LPC bridge Enable the IO bar. xc_io_write to 0x8xxx should work now

; Initial serial port
xc_io_write 0x2e, 0x55
xc_io_write 0x2e, 0x07
xc_io_write 0x2f, 0x04
xc_io_write 0x2e, 0x30
xc_io_write 0x2f, 0x01
xc_io_write 0x2e, 0x61
xc_io_write 0x2f, 0x03F8 & 0xff
xc_io_write 0x2e, 0x60
xc_io_write 0x2f, 0x03F8 >> 8
xc_io_write 0x2e, 0xAA

; Sets up something in MCPX
xc_io_write 0x00008049, 0x00000008 ; ?
xc_io_write 0x000080D9, 0x00000000 ; ?
xc_io_write 0x00008026, 0x00000001 ; ?

; Setup GPU and AGP bus. During xcodes the GPU MMIO base is 0x0F000000 because xcode cannot access high memory regions
xc_pci_write 0x8000F04C, 0x00000001 ; AGP Bridge Enable?
xc_pci_write 0x8000F018, 0x00010100 ; AGP Bridge Set Secondary bus and subordinate bus to 1
xc_pci_write 0x80000084, 0x07FFFFFF ; Host bridge Set memsize to 128mb provisional
xc_pci_write 0x8000F020, 0x0FF00F00 ; AGP Bridge memory limit (0x0FF0) / memory base (0x0F00)
xc_pci_write 0x8000F024, 0xF7F0F000 ; AGP Bridge prefetch memory limit (0xF7F0) / prefetch memory base (0xF000)
xc_pci_write 0x80010010, 0x0F000000 ; NV2A GPU BAR0 - base address (registers) (temporary during xcodes)
xc_pci_write 0x80010014, 0xF0000000 ; NV2A GPU BAR1 - base address ?
xc_pci_write 0x80010004, 0x00000007 ; NV2A enable MMIO, I/O, bus master
xc_pci_write 0x8000F004, 0x00000007 ; AGP Bridge enable MMIO, I/O, bus master

; Setup SMBUS IO
xc_pci_write 0x80000904, 0x00000001 ; SMBUS BAR0 to 1.
xc_pci_write 0x80000914, 0x0000C001 ; SMBUS BAR1 to 0xC001
xc_pci_write 0x80000918, 0x0000C201 ; SMBUS BAR2 to 0xC201
xc_io_write 0x0000C200, 0x00000070  ; Set something in 0xC200

; Sets up peripherals in ISA bridge?
xc_pci_read 0x80000860
xc_andor 0xFFFFFFFF, 0x00000400
xc_chain op_pci_write, 0x80000860
xc_pci_write 0x8000084C, 0x0000FDDE
xc_pci_write 0x8000089C, 0x871CC707
xc_pci_read 0x800008B4
xc_andor 0xFFFFF0FF, 0x00000F00
xc_chain op_pci_write, 0x800008B4

; Do some encoder initialisation
xc_puts "encoder init"
smbus_poke 0x8A, conexant_init
smbus_poke 0xD4, focus_init
smbus_poke 0xE0, xcalibur_init

xcalibur_init:
xc_puts "xcalibur"
; Nothing specific to do it seems
xc_jmp common_init_continue0

focus_init:
xc_puts "focus"
smbus_write_byte 0xD4, 0x0C, 0x00
smbus_write_byte 0xD4, 0x0D, 0x20
xc_jmp common_init_continue0

conexant_init:
xc_puts "conexant"
smbus_write_byte 0x8A, 0xBA, 0x3F
smbus_write_byte 0x00, 0x6C, 0x46
smbus_write_byte 0x00, 0xB8, 0x00
smbus_write_byte 0x00, 0xCE, 0x19
smbus_write_byte 0x00, 0xC6, 0x9C
smbus_write_byte 0x00, 0x32, 0x08
smbus_write_byte 0x00, 0xC4, 0x01
xc_jmp common_init_continue0

common_init_continue0:
; GPU PFB block
xc_mem_write 0x0F100200, 0x03070103
xc_mem_write 0x0F100410, 0x11000016
xc_mem_write 0x0F100330, 0x84848888
xc_mem_write 0x0F10032C, 0xFFFFCFFF
xc_mem_write 0x0F100328, 0x00000001
xc_mem_write 0x0F100338, 0x000000DF
xc_mem_write 0x0F1002D4, 0x00000001
xc_mem_write 0x0F1002C4, 0x00100000
xc_mem_write 0x0F1002CC, 0x00100000
xc_mem_write 0x0F1002C0, 0x00000011
xc_mem_write 0x0F1002C8, 0x00000011
xc_mem_write 0x0F1002C0, 0x00000032
xc_mem_write 0x0F1002C8, 0x00000032
xc_mem_write 0x0F1002C0, 0x00000132
xc_mem_write 0x0F1002C8, 0x00000132
xc_mem_write 0x0F1002D0, 0x00000001
xc_mem_write 0x0F1002D0, 0x00000001
xc_mem_write 0x0F100210, 0x80000000
xc_mem_write 0x0F100228, 0x081205FF

; GPU PRAMDAC block
xc_mem_write 0x0F680500, 0x00011C01         ; NV-PRAMDAC-NVPLL-COEFF
xc_mem_write 0x0F68050C, 0x000A0400         ; NV-PRAMDAC-PLL-COEFF-SELECT

; GPU PBUS block (COMMON - initial timing seed values)
xc_mem_write 0x0F001220, 0x00000000
xc_mem_write 0x0F001228, 0x00000000
xc_mem_write 0x0F001264, 0x00000000
xc_mem_write 0x0F0010B4, 0x00000000
xc_mem_write 0x0F0010D8, 0x00000000
xc_mem_write 0x0F001230, 0xFFFFFFFF
xc_mem_write 0x0F001234, 0xAAAAAAAA
xc_mem_write 0x0F001238, 0xAAAAAAAA
xc_mem_write 0x0F00123C, 0x8B8B8B8B
xc_mem_write 0x0F001240, 0xFFFFFFFF
xc_mem_write 0x0F001244, 0x8B8B8B8B
xc_mem_write 0x0F001248, 0x8B8B8B8B
xc_mem_write 0x0F00124C, 0xAA8BAA8B
xc_mem_write 0x0F001250, 0x00005474
xc_mem_write 0x0F001218, 0x00000000

; Lets determine the MCPX revision. if 0xD5, its a "1.6" xbox. This has slightly different PBUS init
xc_pci_read 0x80000808
xc_jne 0x000000D5, mcpx_10_15

mcpx_16:
xc_puts "mcpx 1.6"
xc_mem_write 0x0F0010B0, 0x01000010
xc_mem_write 0x0F0010CC, 0x66660000
xc_mem_write 0x0F0010D4, 0x0000000F
xc_mem_write 0x0F0010BC, 0x00005860
xc_mem_write 0x0F0010C4, 0xAAAA0000
xc_mem_write 0x0F0010C8, 0x00007D67
xc_mem_write 0x0F0010DC, 0x00000000
xc_mem_write 0x0F0010E8, 0x04000000
xc_mem_write 0x0F001210, 0x00000000

xc_mem_write 0x0F0010B8, 0xDDDD0000
xc_mem_write 0x0F001214, 0x88888888
xc_mem_write 0x0F00122C, 0x64646464
xc_mem_write 0x0F00123C, 0x54545454
xc_mem_write 0x0F001230, 0xF8F8F8F8
xc_mem_write 0x0F001240, 0xF8F8F8F8
xc_mem_write 0x0F001234, 0x87878787
xc_mem_write 0x0F001244, 0x65656565
xc_mem_write 0x0F001238, 0xbbbbbbbb
xc_mem_write 0x0F001248, 0x96969696
xc_mem_write 0x0F001218, 0x00000000
xc_jmp common_init_continue2

mcpx_10_15:
xc_puts "mcpx 1.0-1.5"
xc_mem_write 0x0F0010B0, 0x07633461
xc_mem_write 0x0F0010CC, 0x66660000
xc_mem_write 0x0F0010D4, 0x00000009
xc_mem_write 0x0F0010BC, 0x00005866
xc_mem_write 0x0F0010C4, 0x0351C858
xc_mem_write 0x0F0010C8, 0x30007D67
xc_mem_write 0x0F0010DC, 0xA0423635
xc_mem_write 0x0F0010E8, 0x0C6558C6
xc_mem_write 0x0F001210, 0x00000010

xc_mem_write 0x0F001230, 0xFFFFFFFF
xc_mem_write 0x0F001234, 0xAAAAAAAA
xc_mem_write 0x0F001238, 0xAAAAAAAA
xc_mem_write 0x0F00123C, 0x8B8B8B8B
xc_mem_write 0x0F001240, 0xFFFFFFFF
xc_mem_write 0x0F001244, 0x8B8B8B8B
xc_mem_write 0x0F001248, 0x8B8B8B8B
xc_mem_write 0x0F1002D4, 0x00000001         ; NV04-PFB-PRE
xc_mem_write 0x0F1002C4, 0x00100042
xc_mem_write 0x0F1002CC, 0x00100042
xc_mem_write 0x0F1002C0, 0x00000011
xc_mem_write 0x0F1002C8, 0x00000011
xc_mem_write 0x0F1002C0, 0x00000032
xc_mem_write 0x0F1002C8, 0x00000032
xc_mem_write 0x0F1002C0, 0x00000132
xc_mem_write 0x0F1002C8, 0x00000132
xc_mem_write 0x0F1002D0, 0x00000001         ; NV04-PFB-REF
xc_mem_write 0x0F1002D0, 0x00000001         ; NV04-PFB-REF
xc_mem_write 0x0F100210, 0x80000000
xc_mem_write 0x0F00124C, 0xAA8BAA8B
xc_mem_write 0x0F001250, 0x0000AA8B
xc_mem_write 0x0F100228, 0x081205FF
xc_mem_write 0x0F001218, 0x00010000

; The below logic seems to only be valid for 5713 and below (non 1.6 boards)
; Determine if we have Samsung RAM. or Micron or Hynix RAM and apply specific timings
xc_mem_read 0x0F101000                      ; NV-PEXTDEV-BOOT-0
xc_andor 0x000C0000, 0x00000000
xc_jne 0x00000000, samsung_or_hynix_ram

micron_ram:
xc_io_write 0x03F8, 'm'
xc_io_write 0x03F8, 0x0A
xc_mem_read 0x0F101000
xc_andor 0xE1F3FFFF, 0x80000000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0xEEEE0000

xc_mem_write 0x0F001214, 0x48480848
xc_mem_write 0x0F00122C, 0x88888888
xc_jmp common_init_continue2

samsung_or_hynix_ram:
xc_jne 0x000C0000, hynix_ram
; Looks like there's logic in 5713 to check for a alternate type of Samsung RAM
; xboxdevwiki mentions "Samsung F-die RAM Chips" around this rev so maybe thats it.
xc_io_write 0x000080D0, 0x00000041
xc_io_read 0x000080D0
xc_andor 0x00000020, 0x00000000
xc_jne 0x00000000, samsung_ram_v2

samsung_ram:
xc_io_write 0x03F8, 'S'
xc_io_write 0x03F8, 0x0A
xc_mem_read 0x0F101000
xc_andor 0xE1F3FFFF, 0x860C0000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0xFFFF0000
xc_mem_write 0x0F001214, 0x09090909
xc_mem_write 0x0F00122C, 0xAAAAAAAA
xc_jmp common_init_continue2

samsung_ram_v2:
xc_io_write 0x03F8, 's'
xc_io_write 0x03F8, 0x0A
xc_mem_read 0x0F101000                      ; NV-PEXTDEV-BOOT-0
xc_andor 0xE1F3FFFF, 0x86040000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0xFFFF0000
xc_mem_write 0x0F001214, 0x09090909
xc_mem_write 0x0F00122C, 0xAAAAAAAA
xc_jmp common_init_continue2

hynix_ram:
xc_io_write 0x03F8, 'h'
xc_io_write 0x03F8, 0x0A
xc_mem_read 0x0F101000
xc_andor 0xE1F3FFFF, 0x820C0000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0x11110000
xc_mem_write 0x0F001214, 0x09090909
xc_mem_write 0x0F00122C, 0xAAAAAAAA
xc_jmp common_init_continue2

common_init_continue2:
; Tell the PIC we have good memory test results
smbus_write_byte 0x20, 0x13, 0x0F
smbus_write_byte 0x00, 0x12, 0xF0

; DRAM controller setup before memory test
xc_pci_write 0x80000340, 0xF0F0C0C0
xc_pci_write 0x80000344, 0x00000000
xc_pci_write 0x8000035C, 0x00000000
xc_pci_write 0x8000036C, 0x00230801
xc_pci_write 0x8000036C, 0x01230801

; Memory test on all banks to determine 64MB or 128MB (FIXME)


; Set final GPU base before leaving xcodes
xc_pci_write 0x8000F020, 0xFDF0FD00
xc_pci_write 0x80010010, 0xFD000000

; VISOR trick. Due to rollover bug at 0xFFFFFFFF, use xcodes to push a jmp to our code in ROM at 0xfffc1000
; mov eax, 0xfffc1000 (This is our code entry in ROM)
; jmp eax
; nop
xc_mem_write 0x00000000, 0xfc2000B8
xc_mem_write 0x00000004, 0x90e0ffff

xc_opcode op_exit, 0x806, 0
