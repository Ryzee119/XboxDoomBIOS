; SPDX-License-Identifier: GPL-2.0
; Copyright (c) https://github.com/XboxDev/cromwell

; https://github.com/Ernegien/xdecode
; xdecode -i cromwell.bin -c comments.conf

BITS 32

op_mem_read		EQU 0x02
op_mem_write	EQU 0x03
op_pci_write	EQU 0x04
op_pci_read		EQU 0x05
op_andor		EQU 0x06
op_chain		EQU 0x07
op_jne			EQU 0x08
op_jmp			EQU 0x09
op_andorebp		EQU 0x10
op_io_write		EQU 0x11
op_io_read		EQU 0x12
op_exit			EQU 0xEE
op_unused1		EQU 0xF5
op_unused2		EQU 0x80

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

xc_opcode op_unused1, 0x00081000, 1
xc_opcode op_unused2, 0x00000114, 0x228
xc_pci_write 0x80000884, 0x00008001
xc_pci_write 0x80000810, 0x00008001
xc_pci_write 0x80000804, 0x00000003
xc_io_write 0x00008049, 0x00000008
xc_io_write 0x000080D9, 0x00000000
xc_io_write 0x00008026, 0x00000001
xc_pci_write 0x8000F04C, 0x00000001
xc_pci_write 0x8000F018, 0x00010100
xc_pci_write 0x80000084, 0x07FFFFFF
xc_pci_write 0x8000F020, 0x0FF00F00
xc_pci_write 0x8000F024, 0xF7F0F000
xc_pci_write 0x80010010, 0x0F000000         ; set nv2a register base address
xc_pci_write 0x80010014, 0xF0000000
xc_pci_write 0x80010004, 0x00000007
xc_pci_write 0x8000F004, 0x00000007
xc_mem_write 0x0F0010B0, 0x07633461
xc_mem_write 0x0F0010CC, 0x66660000
xc_mem_read 0x0F101000
xc_andor 0x000C0000, 0x00000000
xc_jne 0x00000000, loc_161
xc_mem_read 0x0F101000
xc_andor 0xE1F3FFFF, 0x80000000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0xEEEE0000
xc_jmp loc_1BB
loc_161:
xc_jne 0x000C0000, loc_197
xc_mem_read 0x0F101000
xc_andor 0xE1F3FFFF, 0x860C0000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0xFFFF0000
xc_jmp loc_1BB
loc_197:
xc_mem_read 0x0F101000
xc_andor 0xE1F3FFFF, 0x820C0000
xc_chain op_mem_write, 0x0F101000
xc_mem_write 0x0F0010B8, 0x11110000
loc_1BB:
xc_mem_write 0x0F0010D4, 0x00000009
xc_mem_write 0x0F0010B4, 0x00000000
xc_mem_write 0x0F0010BC, 0x00005866
xc_mem_write 0x0F0010C4, 0x0351C858
xc_mem_write 0x0F0010C8, 0x30007D67
xc_mem_write 0x0F0010D8, 0x00000000
xc_mem_write 0x0F0010DC, 0xA0423635
xc_mem_write 0x0F0010E8, 0x0C6558C6
xc_mem_write 0x0F100200, 0x03070103
xc_mem_write 0x0F100410, 0x11000016
xc_mem_write 0x0F100330, 0x84848888
xc_mem_write 0x0F10032C, 0xFFFFCFFF
xc_mem_write 0x0F100328, 0x00000001
xc_mem_write 0x0F100338, 0x000000DF
xc_pci_write 0x80000904, 0x00000001         ; smbus controller setup
xc_pci_write 0x80000914, 0x0000C001         ; smbus controller setup
xc_pci_write 0x80000918, 0x0000C201         ; smbus controller setup
xc_io_write 0x0000C200, 0x00000070          ; smbus controller setup
xc_io_write 0x0000C004, 0x0000008A          ; smbus address set ; conexant encoder
xc_io_write 0x0000C008, 0x000000BA          ; smbus command set
xc_io_write 0x0000C006, 0x0000003F          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_281:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_29C
xc_jmp loc_2B7
loc_29C:
xc_andor 0x00000008, 0x00000000
xc_jne 0x00000000, loc_281
xc_jmp loc_40D
loc_2B7:
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x0000006C          ; smbus command set
xc_io_write 0x0000C006, 0x00000046          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_2DB:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_2DB
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x000000B8          ; smbus command set
xc_io_write 0x0000C006, 0x00000000          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_311:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_311
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x000000CE          ; smbus command set
xc_io_write 0x0000C006, 0x00000019          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_347:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_347
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x000000C6          ; smbus command set
xc_io_write 0x0000C006, 0x0000009C          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_37D:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_37D
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x00000032          ; smbus command set
xc_io_write 0x0000C006, 0x00000008          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_3B3:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_3B3
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x000000C4          ; smbus command set
xc_io_write 0x0000C006, 0x00000001          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_3E9:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_3E9
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_jmp loc_548
loc_40D:
xc_io_write 0x0000C000, 0x000000FF          ; smbus status set
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C004, 0x000000D4          ; smbus address set ; focus encoder
xc_io_write 0x0000C008, 0x0000000C          ; smbus command set
xc_io_write 0x0000C006, 0x00000000          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_443:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_45E
xc_jmp loc_479
loc_45E:
xc_andor 0x00000008, 0x00000000
xc_jne 0x00000000, loc_443
xc_jmp loc_4C1
loc_479:
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x0000000D          ; smbus command set
xc_io_write 0x0000C006, 0x00000020          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_49D:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_49D
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_jmp loc_548
loc_4C1:
xc_io_write 0x0000C000, 0x000000FF          ; smbus status set
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C004, 0x000000E0          ; smbus address set
xc_io_write 0x0000C008, 0x00000000          ; smbus command set
xc_io_write 0x0000C006, 0x00000000          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_4F7:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_4F7
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x000000B8          ; smbus command set
xc_io_write 0x0000C006, 0x00000000          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_52D:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_52D
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
loc_548:
xc_io_write 0x0000C004, 0x00000020          ; smbus address set ; PIC
xc_io_write 0x0000C008, 0x00000001          ; smbus command set
xc_io_write 0x0000C006, 0x00000000          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_56C:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_56C
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C004, 0x00000021          ; smbus address set
xc_io_write 0x0000C008, 0x00000001          ; smbus command set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_5A2:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_5A2
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_read 0x0000C006                       ; smbus value get
xc_mem_write 0x0F680500, 0x00011C01
xc_mem_write 0x0F68050C, 0x000A0400
xc_mem_write 0x0F001220, 0x00000000
xc_mem_write 0x0F001228, 0x00000000
xc_mem_write 0x0F001264, 0x00000000
xc_mem_write 0x0F001210, 0x00000010
xc_mem_read 0x0F101000
xc_andor 0x06000000, 0x00000000
xc_jne 0x00000000, loc_632
xc_mem_write 0x0F001214, 0x48480848
xc_mem_write 0x0F00122C, 0x88888888
xc_jmp loc_668
loc_632:
xc_jne 0x06000000, loc_656
xc_mem_write 0x0F001214, 0x09090909
xc_mem_write 0x0F00122C, 0xAAAAAAAA
xc_jmp loc_668
loc_656:
xc_mem_write 0x0F001214, 0x09090909
xc_mem_write 0x0F00122C, 0xAAAAAAAA
loc_668:
xc_mem_write 0x0F001230, 0xFFFFFFFF
xc_mem_write 0x0F001234, 0xAAAAAAAA
xc_mem_write 0x0F001238, 0xAAAAAAAA
xc_mem_write 0x0F00123C, 0x8B8B8B8B
xc_mem_write 0x0F001240, 0xFFFFFFFF
xc_mem_write 0x0F001244, 0x8B8B8B8B
xc_mem_write 0x0F001248, 0x8B8B8B8B
xc_mem_write 0x0F1002D4, 0x00000001
xc_mem_write 0x0F1002C4, 0x00100042
xc_mem_write 0x0F1002CC, 0x00100042
xc_mem_write 0x0F1002C0, 0x00000011
xc_mem_write 0x0F1002C8, 0x00000011
xc_mem_write 0x0F1002C0, 0x00000032
xc_mem_write 0x0F1002C8, 0x00000032
xc_mem_write 0x0F1002C0, 0x00000132
xc_mem_write 0x0F1002C8, 0x00000132
xc_mem_write 0x0F1002D0, 0x00000001
xc_mem_write 0x0F1002D0, 0x00000001
xc_mem_write 0x0F100210, 0x80000000
xc_mem_write 0x0F00124C, 0xAA8BAA8B
xc_mem_write 0x0F001250, 0x0000AA8B
xc_mem_write 0x0F100228, 0x081205FF
xc_mem_write 0x0F001218, 0x00010000
xc_pci_read 0x80000860
xc_andor 0xFFFFFFFF, 0x00000400
xc_chain op_pci_write, 0x80000860
xc_pci_write 0x8000084C, 0x0000FDDE
xc_pci_write 0x8000089C, 0x871CC707
xc_pci_read 0x800008B4
xc_andor 0xFFFFF0FF, 0x00000F00
xc_chain op_pci_write, 0x800008B4
xc_pci_write 0x80000340, 0xF0F0C0C0

; NOTE ON MCPX D5 The following two lines are 0x00000000 
xc_pci_write 0x80000344, 0x00C00000
xc_pci_write 0x8000035C, 0x04070000

xc_pci_write 0x8000036C, 0x00230801
xc_pci_write 0x8000036C, 0x01230801
xc_jmp loc_7B5
loc_7B5:
xc_jmp loc_7BE
loc_7BE:
xc_mem_write 0x0F100200, 0x03070103
xc_mem_write 0x0F100204, 0x11448000
xc_mem_write 0x0F100200, 0x03070003
xc_pci_write 0x8000103C, 0x00000000         ; memtest type clear
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C004, 0x00000020          ; smbus address set ; PIC
xc_io_write 0x0000C008, 0x00000013          ; smbus command set
xc_io_write 0x0000C006, 0x0000000F          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_806:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_806
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_io_write 0x0000C008, 0x00000012          ; smbus command set
xc_io_write 0x0000C006, 0x000000F0          ; smbus value set
xc_io_write 0x0000C002, 0x0000000A          ; smbus control set ; send
loc_83C:
xc_io_read 0x0000C000                       ; smbus status get
xc_jne 0x00000010, loc_83C
xc_io_write 0x0000C000, 0x00000010          ; smbus status set ; clear
xc_pci_write 0x8000F020, 0xFDF0FD00
xc_pci_write 0x80010010, 0xFD000000         ; set nv2a register base address

xc_mem_write 0xFD001220, 0
xc_mem_write 0xFD00122C, 0x64646464
xc_mem_write 0xFD00123C, 0x54545454
xc_mem_write 0xFD001230, 0xF8F8F8F8
xc_mem_write 0xFD001240, 0xF8F8F8F8
xc_mem_write 0xFD001234, 0x87878787
xc_mem_write 0xFD001244, 0x65656565
xc_mem_write 0xFD001238, 0xbbbbbbbb
xc_mem_write 0xFD001248, 0x96969696
xc_mem_write 0xFD001214, 0x88888888
xc_mem_write 0xFD001218, 0x00000000

; VISOR trick. Due to rollover bug at 0xFFFFFFFF, use xcodes to push a jmp to our code in ROM at 0xfffc1000
; mov eax, 0xfffc1000 (This is our code entry in ROM)
; jmp eax
; nop
xc_mem_write 0x00000000, 0xfc2000B8
xc_mem_write 0x00000004, 0x90e0ffff

xc_opcode op_exit, 0x806, 0