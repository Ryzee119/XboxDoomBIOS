#include "main.h"

uint8_t freertos_running = 0;
static ata_bus_t ata_bus;

extern void vPortYieldCall(void);
extern void vPortTimerHandler(void);

static xbox_eeprom_t eeprom;

void boot_linux(const char *linuxcfg_path);

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
    } else {
        printf_ts("[FS] Mounted drive C as FATX\n");
    }

    if (fileio_register_driver('E', &fatx_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_ts("[FS] Error mounting drive E as FATX\n");
    } else {
        printf_ts("[FS] Mounted drive E as FATX\n");
    }
    
#if (1)
    if (fileio_register_driver('D', &iso9660_io, &ata_ll_io, NULL, &ata_bus) != 0) {
        printf_ts("[FS] Error mounting drive D as ISO9660\n");
    } else {
        printf_ts("[FS] Mounted drive D as ISO9660\n");
    }
#endif
#if (1)
    const char *path = "D:/";
    directory_handle_t *dir = opendir(path);
    if (dir) {
        directory_entry_t *entry;
        while ((entry = readdir(dir)) != NULL) {
            printf_ts("[FS] Found file in %s: %s\r\n", path, entry->file_name);
        }
        closedir(dir);
    } else {
        printf_ts("[FS] Failed to open root of %s\n", path);
    }
#endif
    xbox_led_output(XLED_GREEN, XLED_GREEN, XLED_GREEN, XLED_GREEN);

    boot_linux("D:/linuxboot.cfg");

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
    printf("gettimeofday\n");
    return -1;
}
