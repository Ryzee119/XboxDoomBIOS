// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2024 Ryzee119

// https://archive.org/details/amd-756

#include "xbox.h"

void xbox_pci_init(void)
{
    uint32_t value;
    uint32_t pci_mem[256 / 4];
    pci_header_t *h = (pci_header_t *)pci_mem;

    __asm__ __volatile__("cli");

#if (1)
    pci_io_output_byte(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x00000080, 0x02); // v1.1 2BL kill ROM area

    // Cromwell does this in asm - says it enables IDE and NIC
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x0000008c, 0x40000000);

    // ? Seen when booting 5834
    value = pci_io_input_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x00000054);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x00000054, value | 0x88000000);

    value = pci_io_input_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x00000064);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x00000064, value | 0x88000000);

    value = pci_io_input_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x0000006c);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x0000006c, value & 0xFFFFFFFE);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x0000006c, value);

    // Cromwell - CPU Whoami   ? sesless ?
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x00000080, 0x00000100);

    uint8_t mcpx_version = pci_io_input_byte(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x8);
    if (mcpx_version >= 0xd1) {
        pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0xc8, 0x8f00); // v1.1 2BL <-- death
    }

    // Set up the Host bridge (Cromwell - I dont see all this in retail)
    //pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x48, 0x00000114); -> repeated below
    //pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x44, 0x80000000); -> never seen
    //pci_io_output_byte(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x87, 0x03);       // 64MB Top limit (0x07 for 128MB)
    //pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x84, 0x3FFFFFF); // 64MB top limit (Set in xcodes)

    // Start of retail stuff
    // Set up the LPC bridge
    pci_io_output_byte(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x6a, 0x03);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x6c, 0x0e065491);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x64, 0x00000b0c);
    value = pci_io_input_byte(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x81);
    pci_io_output_byte(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x81, value |= 0x08);

    // IDE Controller
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_IDE_DEVICE_ID, PCI_IDE_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MASTER;
    h->type_0.base_address[4] = PCI_IDE_IO_REGISTER_BASE_4 | PCI_COMMAND_IO;
    pci_mem[0x50 / 4] = 0x00000002;
    pci_mem[0x58 / 4] = 0x20202020;
    pci_mem[0x60 / 4] = 0xC0C0C0C0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_IDE_DEVICE_ID, PCI_IDE_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);

    // Network controller
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_NIC_DEVICE_ID, PCI_NIC_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_0.base_address[0] = PCI_NIC_MEMORY_REGISTER_BASE_0;
    h->type_0.base_address[1] = PCI_NIC_IO_REGISTER_BASE_1 | PCI_COMMAND_IO;
    h->type_0.interrupt_line = XBOX_PIC_NIC_IRQ;
    h->type_0.interrupt_pin = 0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_NIC_DEVICE_ID, PCI_NIC_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);

    // USB0
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_USB0_DEVICE_ID, PCI_USB0_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_0.base_address[0] = PCI_USB0_MEMORY_REGISTER_BASE_0;
    h->type_0.interrupt_line = XBOX_PIC_USB0_IRQ;
    h->type_0.interrupt_pin = 0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_USB0_DEVICE_ID, PCI_USB0_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_USB0_DEVICE_ID, PCI_USB0_FUNCTION_ID, 0x50, 0x0000000f); // Port Enable?

    // USB1
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_USB1_DEVICE_ID, PCI_USB1_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_0.base_address[0] = PCI_USB1_MEMORY_REGISTER_BASE_0;
    h->type_0.interrupt_line = XBOX_PIC_USB1_IRQ;
    h->type_0.interrupt_pin = 0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_USB1_DEVICE_ID, PCI_USB1_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_USB1_DEVICE_ID, PCI_USB1_FUNCTION_ID, 0x50, 0x00000030);

    // ACI
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_ACI_DEVICE_ID, PCI_ACI_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_0.base_address[0] = PCI_ACI_IO_REGISTER_BASE_0 | PCI_COMMAND_IO;
    h->type_0.base_address[1] = PCI_ACI_IO_REGISTER_BASE_1 | PCI_COMMAND_IO;
    h->type_0.base_address[2] = PCI_ACI_MEMORY_REGISTER_BASE_2;
    h->type_0.interrupt_line = XBOX_PIC_ACI_IRQ;
    h->type_0.interrupt_pin = 0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_ACI_DEVICE_ID, PCI_ACI_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);

    // APU
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_APU_DEVICE_ID, PCI_APU_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_0.base_address[0] = PCI_APU_MEMORY_REGISTER_BASE_0;
    h->type_0.interrupt_line = XBOX_PIC_APU_IRQ;
    h->type_0.interrupt_pin = 0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_APU_DEVICE_ID, PCI_APU_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);

    // ?
    value = pci_io_input_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x8c);
    value &= 0xF3FFFFFF;
    value |= 0x08000000;
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_LPCBRIDGE_DEVICE_ID, PCI_LPCBRIDGE_FUNCTION_ID, 0x8c, value);

    // ?
    io_output_byte(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0xCD, 0x08);

    // ?
    value = pci_io_input_dword(PCI_XBOX_SYSTEM_BUS, PCI_ACI_DEVICE_ID, PCI_ACI_FUNCTION_ID, 0x4c);
    value |= 0x01010000;
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_ACI_DEVICE_ID, PCI_ACI_FUNCTION_ID, 0x4c, value);

    // AGP
    pci_io_input_n(PCI_XBOX_SYSTEM_BUS, PCI_AGPBRIDGE_DEVICE_ID, PCI_AGPBRIDGE_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_1.primary_bus_number = 0;
    h->type_1.secondary_bus_number = 1;
    h->type_1.subordinate_bus_number = 1;
    h->type_1.memory_base = 0xFD00;
    h->type_1.memory_limit = 0xFE70;
    h->type_1.prefetchable_memory_base = 0xF000;
    h->type_1.prefetchable_memory_limit = 0xF3F0;
    pci_io_output_n(PCI_XBOX_SYSTEM_BUS, PCI_AGPBRIDGE_DEVICE_ID, PCI_AGPBRIDGE_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);

    // ?
    pci_io_output_dword(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x48, 0x00000114);

    // GPU
    pci_io_input_n(PCI_XBOX_GPU_BUS, PCI_GPU_DEVICE_ID, PCI_GPU_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);
    h->command |= PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
    h->type_0.base_address[0] = PCI_GPU_MEMORY_REGISTER_BASE_0;
    h->type_0.interrupt_line = XBOX_PIC_GPU_IRQ;
    h->type_0.interrupt_pin = 1;
    pci_mem[0x4C / 4] = 0x00000114;
    pci_io_output_n(PCI_XBOX_GPU_BUS, PCI_GPU_DEVICE_ID, PCI_GPU_FUNCTION_ID, sizeof(pci_mem), (uint8_t *)pci_mem);

    // ??
    io_output_word(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x04, io_input_word((PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x04)) | 0x0001);
    io_output_word(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x22, io_input_word((PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x22)) | 0x0002);
    io_output_word(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x23, io_input_word((PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x23)) | 0x0002);
    io_output_word(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x02, io_input_word((PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x02)) | 0x0001);
    io_output_word(PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x28, io_input_word((PCI_LPCBRIDGE_IO_REGISTER_BASE_0 + 0x28)) | 0x0001);

    // 64MB Top limit (0x07 for 128MB)
    pci_io_output_byte(PCI_XBOX_SYSTEM_BUS, PCI_HOSTBRIDGE_DEVICE_ID, PCI_HOSTBRIDGE_FUNCTION_ID, 0x87, 0x03);

    //?
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x1B, 0x04); // Scratch register
    //smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x0b, 0x01); // Audio mute
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x19, 0x01); // No reset on eject
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x1A, 0x01); // Enable SMC interrupt
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, 0x0B, 0x00); // Allow audio

    //? Seen later in booting into msdash
    value = pci_io_input_dword(PCI_XBOX_GPU_BUS, PCI_GPU_DEVICE_ID, PCI_GPU_FUNCTION_ID, 0x0000004c);
    pci_io_output_dword(PCI_XBOX_GPU_BUS, PCI_GPU_DEVICE_ID, PCI_GPU_FUNCTION_ID, 0x0000004c, 0x1f000000 | value);
#endif
}

#if (0)
// Function to enumerate PCI devices
static void enumerate_pci_devices()
{
    uint8_t bus, dev, func;

    // https://xboxdevwiki.net/Porting_an_Operating_System_to_the_Xbox_HOWTO#PCI_Enumeration_Bug
    // dev starts from 1 to avoid the enumeration bug
    for (bus = 0; bus < 255; ++bus) {          // Assume up to 256 buses
        for (dev = 1; dev < 32; ++dev) {       // Assume up to 32 devices per bus
            for (func = 0; func < 8; ++func) { // Assume up to 8 functions per device
                uint16_t vendor_id = pci_io_input_word(bus, dev, func, 0x00);
                uint16_t device_id = pci_io_input_word(bus, dev, func, 0x02);

                // Check if the vendor ID is 0xFFFF, indicating no device present
                if (vendor_id == 0xFFFF) {
                    continue;
                }

                XPRINTF("Bus: %u, Device: %u, Function: %u\n", bus, dev, func);
                XPRINTF("  Vendor ID: 0x%04X\n", vendor_id);
                XPRINTF("  Device ID: 0x%04X\n", device_id);

                for (int i = 0; i < 0xff; i += 4) {
                    uint32_t data = pci_io_input_dword(bus, dev, func, i);
                    XPRINTF("  %02X.%02x:%01x.%02x: %08X\n", bus, dev, func, i, data);
                }
            }
        }
    }
}

void ReadPciConfigSpace(uint8_t bus, uint8_t device, uint8_t function)
{

    uint16_t vendor_id = pci_io_input_dword(bus, device, function, 0x00);
    if (vendor_id == 0xFFFF) {
        return;
    }
    for (uint16_t offset = 0; offset < 256; offset += 4) {
        uint32_t address = (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset);
        uint32_t value = pci_io_input_dword(bus, device, function, offset);
        // Print the 4 bytes read from the PCI configuration space
        XPRINTF("Bus: %02x, Device: %02x, Function: %02x, Offset: %02x, Data: %08x\r\n", bus, device, function, offset, value);
        // xbox_timer_spin_wait(100 * xbox_timer_query_performance_frequency() / 1000);
    }
}

void ScanPciBus()
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 1; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                // Check if the device exists
                ReadPciConfigSpace(bus, device, function);
            }
        }
    }
}
#endif