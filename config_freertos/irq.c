#include <stdint.h>
#include <stdio.h>

#if (0)
// generate a generation protection fault
__asm__ volatile("mov $0xdead, %ax\n\t"
                    "mov %ax, %ds\n\t"
);

// Generate a divide by zero exception
__asm__ volatile("mov $0, %eax\n\t"
                    "div %eax\n\t"
                    "nop\n\t"
);

// Generate Invalid Opcode
__asm__ volatile("ud2\n\t");
#endif

static const char *exception_fmt = "ERROR_CODE: %08x, EIP: %08X CS: %04X EFLAGS: %08X\n";

typedef struct x86_exception_frame {
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} x86_exception_frame_t;

void __attribute__((weak)) exception_handler0(x86_exception_frame_t exception_frame)
{
    printf("DIV0 EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler1(x86_exception_frame_t exception_frame)
{
    printf("DEBUG EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler2(x86_exception_frame_t exception_frame)
{
    printf("NMI EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler3(x86_exception_frame_t exception_frame)
{
    printf("BREAKPOINT EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler4(x86_exception_frame_t exception_frame)
{
    printf("OVERFLOW EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler5(x86_exception_frame_t exception_frame)
{
    printf("BOUND RANGE EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler6(x86_exception_frame_t exception_frame)
{
    printf("INVALID OPCODE EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler7(x86_exception_frame_t exception_frame)
{
    printf("DEVICE NOT AVAILABLE EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler8(x86_exception_frame_t exception_frame)
{
    printf("DOUBLE FAULT EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler9(x86_exception_frame_t exception_frame)
{
    printf("COPROCESSOR SEGMENT OVERRUN EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler10(x86_exception_frame_t exception_frame)
{
    printf("INVALID TSS EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler11(x86_exception_frame_t exception_frame)
{
    printf("SEGMENT NOT PRESENT EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler12(x86_exception_frame_t exception_frame)
{
    printf("STACK SEGMENT FAULT EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler13(x86_exception_frame_t exception_frame)
{
    printf("GENERAL PROTECTION FAULT EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler14(x86_exception_frame_t exception_frame)
{
    printf("PAGE FAULT EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler15(x86_exception_frame_t exception_frame)
{
    printf("RESERVED EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler16(x86_exception_frame_t exception_frame)
{
    printf("x87 FLOATING POINT ERROR EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) exception_handler17(x86_exception_frame_t exception_frame)
{
    printf("ALIGNMENT CHECK EXCEPTION\n");
    printf(exception_fmt, exception_frame.error_code, exception_frame.eip, exception_frame.cs, exception_frame.eflags);
}

void __attribute__((weak)) timer_handler()
{
    printf("TIMER\n");
}

void __attribute__((weak)) usb0_handler()
{
    printf("USB0\n");
}

void __attribute__((weak)) gpu_handler()
{
    printf("GPU\n");
}

void __attribute__((weak)) nic_handler()
{
    printf("NIC\n");
}

void __attribute__((weak)) apu_handler()
{
    printf("APU\n");
}

void __attribute__((weak)) aci_handler()
{
    printf("NIC\n");
}

void __attribute__((weak)) usb1_handler()
{
    printf("USB1\n");
}

void __attribute__((weak)) ide_handler()
{
    printf("IDE\n");
}

void __attribute__((weak)) smc_handler()
{
    uint8_t irq_reason;
    #if (0)
    smbus_input_byte(0x20, 0x11, &irq_reason);
    printf("[SMC] IRQ Reason is 0x%02x\n", irq_reason);

    if (irq_reason & (1 << 0)) {
        printf("SMC: POWERDOWN\n");
    }

    if (irq_reason & (1 << 1)) {
        printf("SMC: TRAY CLOSED\n");
    }

    if (irq_reason & (1 << 5)) {
        printf("SMC: EJECT PRESSED\n");
        smbus_output_byte(0x20, 0x0D, 0x04);
        smbus_output_byte(0x20, 0x0C, 0x00);

    }

    if (irq_reason & (1 << 2)) {
        printf("SMC: CDROM IS NOW OPENING\n");
        smbus_output_byte(0x20, 0x0D, 0x02);
    }

    if (irq_reason & (1 << 3)) {
        printf("SMC: AV CABLE CONNECTED\n");
    }

    if (irq_reason & (1 << 4)) {
        printf("SMC: AV CABLE UNPLUGGED\n");
    }

    if (irq_reason & (1 << 6)) {
        printf("SMC: TRAY CLOSING\n");
    }
    #endif

}
