#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// Minimal ELF 32-bit structures to keep code self-contained
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;

#define EI_NIDENT 16
#define PT_LOAD   1

typedef struct {
    unsigned char e_ident[EI_NIDENT]; // Magic number and other info
    Elf32_Half    e_type;             // Object file type
    Elf32_Half    e_machine;          // Architecture
    Elf32_Word    e_version;          // Object file version
    Elf32_Addr    e_entry;            // Entry point virtual address
    Elf32_Off     e_phoff;            // Program header table file offset
    Elf32_Off     e_shoff;            // Section header table file offset
    Elf32_Word    e_flags;            // Processor-specific flags
    Elf32_Half    e_ehsize;           // ELF header size in bytes
    Elf32_Half    e_phentsize;        // Program header table entry size
    Elf32_Half    e_phnum;            // Program header table entry count
    Elf32_Half    e_shentsize;        // Section header table entry size
    Elf32_Half    e_shnum;            // Section header table entry count
    Elf32_Half    e_shstrndx;         // Section header string table index
} Elf32_Ehdr;

typedef struct {
    Elf32_Word p_type;   // Segment type
    Elf32_Off  p_offset; // Segment file offset
    Elf32_Addr p_vaddr;  // Segment virtual address
    Elf32_Addr p_paddr;  // Segment physical address
    Elf32_Word p_filesz; // Segment size in file
    Elf32_Word p_memsz;  // Segment size in memory
    Elf32_Word p_flags;  // Segment flags
    Elf32_Word p_align;  // Segment alignment
} Elf32_Phdr;

// Function to load the ELF and return the entry point address
uint32_t load_elf_payload(const char* filepath) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        printf("Error: Could not open %s\n", filepath);
        return 0;
    }

    // Read the ELF Header
    Elf32_Ehdr elf_hdr;
    if (fread(&elf_hdr, sizeof(Elf32_Ehdr), 1, file) != 1) {
        printf("Error: Failed to read ELF header.\n");
        fclose(file);
        return 0;
    }

    // Verify ELF Magic Bytes: 0x7F, 'E', 'L', 'F'
    if (memcmp(elf_hdr.e_ident, "\x7F""ELF", 4) != 0) {
        printf("Error: File is not a valid ELF.\n");
        fclose(file);
        return 0;
    }

    // Verify it is a 32-bit ELF (Class 1)
    if (elf_hdr.e_ident[4] != 1) {
        printf("Error: Not a 32-bit ELF.\n");
        fclose(file);
        return 0;
    }

    // Seek to the Program Headers
    fseek(file, elf_hdr.e_phoff, SEEK_SET);

    // Read and process each Program Header
    for (int i = 0; i < elf_hdr.e_phnum; i++) {
        Elf32_Phdr phdr;
        fseek(file, elf_hdr.e_phoff + (i * elf_hdr.e_phentsize), SEEK_SET);
        fread(&phdr, sizeof(Elf32_Phdr), 1, file);

        // We only care about PT_LOAD segments (Loadable segments)
        if (phdr.p_type == PT_LOAD) {
            
            // Pointer to the physical memory location requested by the ELF
            // WARNING: In a user-space OS (Windows/Linux), this cast will cause a segfault.
            // This is meant to be executed in a bare-metal Ring-0 bootloader.
            uint8_t *dest = (uint8_t *)phdr.p_paddr;

            printf("Loading segment %d: File Offset 0x%X -> Mem Addr 0x%X (File Size: %u, Mem Size: %u)\n", 
                   i, phdr.p_offset, phdr.p_paddr, phdr.p_filesz, phdr.p_memsz);

            // 1. Read the initialized data (.text, .data) from the file into memory
            fseek(file, phdr.p_offset, SEEK_SET);
            fread(dest, 1, phdr.p_filesz, file);

            // 2. Zero out the uninitialized data (.bss)
            // The ELF spec dictates that if memory size > file size, the remainder must be zeroed.
            if (phdr.p_memsz > phdr.p_filesz) {
                memset(dest + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
            }
        }
    }

    fclose(file);
    return elf_hdr.e_entry; // Return the execution entry point
}
