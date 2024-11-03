#include "main.h"

uint8_t freertos_running = 0;
static ata_bus_t ata_bus;

extern void vPortYieldCall(void);
extern void vPortTimerHandler(void);

static void doom_task(void *parameters)
{
    while (1) {
        doom_entry("C:/doom1.wad");
    }
}

static xbox_eeprom_t eeprom;


void draw_rect(uint32_t color, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    const display_information_t *display = xbox_video_get_display_information();
    if (!display->frame_buffer) {
        return;
    }

    uint32_t *fb32 = (uint32_t *)display->frame_buffer;
    for (uint32_t i = 0; i < height; i++) {
        for (uint32_t j = 0; j < width; j++) {
            fb32[(y + i) * display->width + x + j] = color;

        }
    }
    xbox_video_flush_cache();
}

static void freertos_entry(void *parameters)
{
    (void)parameters;
    printf("FreeRTOS entry\n");

    // FreeRTOS x86 port uses the LAPIC timer. This must be used in-conjunction with the IOAPIC interrupt controller.
    // Although the xbox should have both of these peripherals, I could not get the IOAPIC to work. I suspect there is
    // some PCI address space write to enable it (where!?). Also APU overlays IOAPIC address space (fixable).
    // Therefore we replace the timer callbacks with the PIC timer interrupt and disable the APIC
    mmio_output_dword(XBOX_APIC_BASE + APIC_LVT_LINT0, 0x00000700);
    mmio_output_dword(XBOX_APIC_BASE + APIC_SIV, 0);
    xPortInstallInterruptHandler(vPortTimerHandler, XBOX_PIC1_BASE_VECTOR_ADDRESS + XBOX_PIT_TIMER_IRQ);
    xbox_interrupt_enable(XBOX_PIT_TIMER_IRQ, 1);

    freertos_running = 1;
    xbox_led_output(XLED_RED, XLED_RED, XLED_RED, XLED_RED);

    display_init();

    interrupts_init();

    xbox_interrupt_enable(XBOX_PIC_SMC_IRQ, 1);

    usb_init();

    ide_bus_init(XBOX_ATA_BUSMASTER_BASE, XBOX_ATA_PRIMARY_BUS_CTRL_BASE, XBOX_ATA_PRIMARY_BUS_IO_BASE, &ata_bus);

    printf_ts("[FS] Mounting filesystems\n");

#if (0)
    cpuid_eax_01 cpuid_info;
    cpu_read_cpuid(CPUID_VERSION_INFO, &cpuid_info.eax.flags, &cpuid_info.ebx.flags, &cpuid_info.ecx.flags,
                   &cpuid_info.edx.flags);

    printf_ts("[CPU] Family: %d\n", cpuid_info.eax.family_id);
    printf_ts("[CPU] Model: %d\n", cpuid_info.eax.model);
    printf_ts("[CPU] Stepping: %d\n", cpuid_info.eax.stepping_id);
    printf_ts("[CPU] Type: %d\n", cpuid_info.eax.processor_type);
    printf_ts("[CPU] Extended Family: %d\n", cpuid_info.eax.extended_family_id);
    printf_ts("[CPU] Extended Model: %d\n", cpuid_info.eax.extended_model_id);
    printf_ts("[CPU] Feature Bits (EDX): 0x%08x\n", cpuid_info.edx.flags);
    printf_ts("[CPU] Feature Bits (ECX): 0x%08x\n", cpuid_info.ecx.flags);
#endif

    extern fs_io_ll_t ata_ll_io;
    extern fs_io_ll_t usb_ll_io;
    extern fs_io_t fat_io;
    extern fs_io_t fatx_io;
    extern fs_io_t iso9660_io;

    if (fileio_register_driver('C', &fatx_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_ts("[FS] Error mounting drive C as FATX\n");
    }

    if (fileio_register_driver('E', &fatx_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_ts("[FS] Error mounting drive E as FATX\n");
    }

    if (fileio_register_driver('D', &iso9660_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_ts("[FS] Error mounting drive D as ISO9660\n");
    }

    xbox_led_output(XLED_GREEN, XLED_GREEN, XLED_GREEN, XLED_GREEN);

    xTaskCreate(doom_task, "Doom!", configMINIMAL_STACK_SIZE * 2, NULL, THREAD_PRIORITY_NORMAL, NULL);

    // We are done here. Delete this task.
    vTaskDelete(NULL);
    return;
}

int main(void)
{
    // We create this task statically; should always succeed.
    // FreeRTOS calls freertos_entry immediately after vTaskStartScheduler without any tick
    // which is good because we can setup the PIC timer with FreeRTOS context before the scheduler actually starts.
    static StaticTask_t freertos_entry_task;
    static StackType_t freertos_entry_stack[configMINIMAL_STACK_SIZE];
    xTaskCreateStatic(freertos_entry, "FreeRTOS!", configMINIMAL_STACK_SIZE, NULL, THREAD_PRIORITY_NORMAL,
                      freertos_entry_stack, &freertos_entry_task);
    vTaskStartScheduler();

    // Should never get here
    assert(0);
    return 0;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("Stack overflow in task %s\n", pcTaskName);
}

void _exit(int code)
{
    while (1)
        ;
}

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    return -1;
}
