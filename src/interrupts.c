#include "main.h"
#include <stdio.h>

void interrupts_init()
{
    // For exceptions we cant use FreeRTOS API (xPortInstallInterruptHandler), so we need to install directly
    idt_install_interrupt(0, freertos_exception0, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(1, freertos_exception1, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(2, freertos_exception2, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(3, freertos_exception3, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(4, freertos_exception4, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(5, freertos_exception5, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(6, freertos_exception6, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(7, freertos_exception7, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(8, freertos_exception8, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(9, freertos_exception9, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(10, freertos_exception10, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(11, freertos_exception11, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(12, freertos_exception12, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(13, freertos_exception13, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(14, freertos_exception14, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(15, freertos_exception15, IDT_PRESENT | IDT_TYPE_INT_GATE);
    idt_install_interrupt(16, freertos_exception16, IDT_PRESENT | IDT_TYPE_INT_GATE);

    xPortInstallInterruptHandler(freertos_usb0_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_USB0_IRQ));
    xPortInstallInterruptHandler(freertos_gpu_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_GPU_IRQ));
    xPortInstallInterruptHandler(freertos_nic_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_NIC_IRQ));
    xPortInstallInterruptHandler(freertos_apu_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_APU_IRQ));
    xPortInstallInterruptHandler(freertos_aci_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_ACI_IRQ));
    xPortInstallInterruptHandler(freertos_usb1_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_USB1_IRQ));
    xPortInstallInterruptHandler(freertos_smc_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_SMC_IRQ));
    xPortInstallInterruptHandler(freertos_ide_interrupt, XBOX_PIC_IRQ_TO_VECTOR(XBOX_PIC_IDE_IRQ));
}
