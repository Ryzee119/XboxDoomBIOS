// SPDX-License-Identifier: CC0-1.0

#include <stdint.h>

#include "pci_io.h"

uint8_t pci_io_input_byte(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    io_output_dword(PCI_CONFIG_ADDRESS,
                    0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC));
    return io_input_byte(PCI_CONFIG_DATA + (reg & 3));
}

uint16_t pci_io_input_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    io_output_dword(PCI_CONFIG_ADDRESS,
                    0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC));
    return io_input_word(PCI_CONFIG_DATA + (reg & 2));
}

uint32_t pci_io_input_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    io_output_dword(PCI_CONFIG_ADDRESS,
                    0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC));
    return io_input_dword(PCI_CONFIG_DATA);
}

void pci_io_output_byte(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint8_t val)
{
    io_output_dword(PCI_CONFIG_ADDRESS,
                    0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC));
    io_output_byte(PCI_CONFIG_DATA + (reg & 3), val);
}

void pci_io_output_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t val)
{
    io_output_dword(PCI_CONFIG_ADDRESS,
                    0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC));
    io_output_word(PCI_CONFIG_DATA + (reg & 2), val);
}

void pci_io_output_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val)
{
    io_output_dword(PCI_CONFIG_ADDRESS,
                    0x80000000 | ((uint32_t)bus << 16) | ((uint32_t)dev << 11) | ((uint32_t)func << 8) | (reg & 0xFC));
    io_output_dword(PCI_CONFIG_DATA, val);
}

void pci_io_input_n(uint8_t bus, uint8_t dev, uint8_t func, uint32_t size_of_data, void *data)
{
    uint32_t i = 0;
    uint32_t *data32 = (uint32_t *)data;
    while (size_of_data >= 4) {
        *data32++ = pci_io_input_dword(bus, dev, func, i);
        size_of_data -= 4;
        i += 4;
    }
    uint8_t *data8 = (uint8_t *)data;
    while (size_of_data > 0) {
        *data8++ = pci_io_input_byte(bus, dev, func, i);
        size_of_data--;
        i++;
    }
}

void pci_io_output_n(uint8_t bus, uint8_t dev, uint8_t func, uint32_t size_of_data, void *data)
{
    uint32_t i = 0;
    uint32_t *data32 = (uint32_t *)data;
    while (size_of_data >= 4) {
        pci_io_output_dword(bus, dev, func, i, *data32++);
        size_of_data -= 4;
        i += 4;
    }
    uint8_t *data8 = (uint8_t *)data;
    while (size_of_data > 0) {
        pci_io_output_byte(bus, dev, func, i, *data8++);
        size_of_data--;
        i++;
    }
}
