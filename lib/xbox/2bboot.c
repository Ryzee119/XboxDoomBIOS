// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

void main(void);

// Sections marked __attribute__((section(".boot_code"))) are not compressed in the ROM image
// This is used for code that must run before the decompressor is initialized and is useful for code that must
// run very quickly after boot, such as the SMC challenge/response

__attribute__((section(".boot_code"))) static void smc_io(uint8_t command, uint8_t read, void *data)
{
    uint8_t *data8 = (uint8_t *)data;

    if (read) {
        read = 1;
    }

    const uint16_t SMBUS_IO_BASE = PCI_SMBUS_IO_REGISTER_BASE_1;

    while (io_input_byte(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

smbus_collision_retry:
    io_output_byte(SMBUS_ADDRESS, XBOX_SMBUS_ADDRESS_SMC | read);
    io_output_byte(SMBUS_COMMAND, command);
    io_output_word(SMBUS_STATUS, 0xFFFF);

    if (read == 0) {
        io_output_byte(SMBUS_DATA, *data8);
    }

    io_output_byte(SMBUS_CONTROL, SMBUS_CONTROL_TRANSFER_TYPE_BYTE | SMBUS_CONTROL_START);

    while (io_input_byte(SMBUS_STATUS) & SMBUS_STATUS_BUSY)
        ;

    // Check for collision
    uint16_t status = io_input_word(SMBUS_STATUS);
    if (status & SMBUS_STATUS_COLLISION) {
        goto smbus_collision_retry;
    }

    if (read) {
        *data8 = io_input_byte(SMBUS_DATA);
    }
    return;
}

__attribute__((section(".boot_code"))) void boot_pic_challenge_response(void)
{
    uint8_t bC, bD, bE, bF;

    smc_io(0x1C, SMBUS_READ, &bC);
    smc_io(0x1D, SMBUS_READ, &bD);
    smc_io(0x1E, SMBUS_READ, &bE);
    smc_io(0x1F, SMBUS_READ, &bF);

    uint8_t b1 = 0x33;
    uint8_t b2 = 0xED;
    uint8_t b3 = ((bC << 2) ^ (bD + 0x39) ^ (bE >> 2) ^ (bF + 0x63));
    uint8_t b4 = ((bC + 0x0b) ^ (bD >> 2) ^ (bE + 0x1b));
    uint8_t n = 4;

    while (n--) {
        b1 += b2 ^ b3;
        b2 += b1 ^ b4;
    }

    uint16_t w = ((uint16_t)b2 << 8) | b1;

    uint8_t w1, w2;
    w1 = (w >> 0) & 0xFF;
    w2 = (w >> 8) & 0xFF;
    smc_io(0x20, SMBUS_WRITE, &w1);
    smc_io(0x21, SMBUS_WRITE, &w2);
    return;
}

__attribute__((section(".boot_code"))) void *boot_memset(void *dest, int c, size_t len)
{
    char *d = dest;
    while (len--) {
        *d++ = c;
    }
    return dest;
}

__attribute__((section(".boot_code"))) void *boot_memcpy(void *dest, const void *src, size_t len)
{
    char *d = dest;
    const char *s = src;
    while (len--) {
        *d++ = *s++;
    }
    return dest;
}

__attribute__((section(".boot_code"))) void *boot_memmove(void *dest, const void *src, size_t len)
{
    char *d = dest;
    const char *s = src;
    if (d < s) {
        while (len--) {
            *d++ = *s++;
        }
    } else {
        const char *lasts = s + (len - 1);
        char *lastd = d + (len - 1);
        while (len--) {
            *lastd-- = *lasts--;
        }
    }
    return dest;
}

#define LZ4_FREESTANDING            1
#define LZ4_memmove(dst, src, size) boot_memmove((dst), (src), (size))
#define LZ4_memcpy(dst, src, size)  boot_memcpy((dst), (src), (size))
#define LZ4_memset(p, v, s)         boot_memset((p), (v), (s))
#define LZ4_FORCE_O2                __attribute__((section(".boot_code")))
__attribute__((section(".boot_code"))) void *lz4_error_memory_allocation_is_disabled(void)
{
    return NULL;
}
#include "lz4/lz4.c"

extern void __libc_init_array(void);
void boot(void)
{
    // Calls functions defined with __attribute__((constructor))
    __libc_init_array();

    xbox_led_output(XLED_RED, XLED_RED, XLED_RED, XLED_RED);

    smbus_init(PCI_SMBUS_IO_REGISTER_BASE_1);

    xbox_serial_init();

    cpu_disable_cache();
    cpu_update_microcode();
    cpu_enable_cache();

    xbox_pci_init();

    // Enable the 8259 Interrupt Controllers (Master and Slave)
    // All IRQs initially masked except IRQ2 (To ensure PIC2 IRQs work)
    io_output_byte(XBOX_PIC1_COMMAND_PORT, ICW1_INIT | ICW1_INTERVAL4 | ICW1_ICW4);
    io_output_byte(XBOX_PIC1_DATA_PORT, XBOX_PIC1_BASE_VECTOR_ADDRESS);
    io_output_byte(XBOX_PIC1_DATA_PORT, ICW3_MASTER_SLAVE);
    io_output_byte(XBOX_PIC1_DATA_PORT, ICW4_8086_MODE);
    io_output_byte(XBOX_PIC1_DATA_PORT, ~(1 << XBOX_PIC_SLAVE_IRQ));

    io_output_byte(XBOX_PIC2_COMMAND_PORT, ICW1_INIT | ICW1_INTERVAL4 | ICW1_ICW4);
    io_output_byte(XBOX_PIC2_DATA_PORT, XBOX_PIC2_BASE_VECTOR_ADDRESS);
    io_output_byte(XBOX_PIC2_DATA_PORT, ICW3_SLAVE_2);
    io_output_byte(XBOX_PIC2_DATA_PORT, ICW4_8086_MODE);
    io_output_byte(XBOX_PIC2_DATA_PORT, OCW1_MASK_ALL);

    // Clear latent SMC Interrupts
    uint8_t smc_irq_status;
    smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_IRQ, &smc_irq_status);

    // Ensure that tray is closed
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_TRAY_CLOSED, 1);

    // Enable PIT timer at 1kHz
    io_output_byte(XBOX_PIT_COMMAND_PORT, PIT_ACCESS_LOHIBYTE | PIT_MODE_SQUARE_WAVE | (XBOX_PIT_CHANNEL0 & 0x0F));
    uint16_t diviser = PIC_TIMER_FREQ / 1000;
    io_output_byte(XBOX_PIT_CHANNEL0, diviser & 0xFF);
    io_output_byte(XBOX_PIT_CHANNEL0, (diviser >> 8) & 0xFF);

    // Disable APIC so PIC IRQs work
    ia32_apic_base_register apic_base;
    cpu_read_msr64(MSR_IA32_APIC_BASE, &apic_base.flags);
    apic_base.apic_global_enable = 0;
    cpu_write_msr64(MSR_IA32_APIC_BASE, apic_base.flags);
    mmio_output_dword(XBOX_APIC_BASE + APIC_LVT_LINT0, 0x00000700);
    mmio_output_dword(XBOX_APIC_BASE + APIC_SIV, 0x00000000);

    xbox_led_output(XLED_ORANGE, XLED_ORANGE, XLED_ORANGE, XLED_ORANGE);

    __asm__("sti");
    __asm__("jmp main");
}
