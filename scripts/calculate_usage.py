import argparse
from elftools.elf.elffile import ELFFile

def print_memory_usage(elf_path):
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)

        memory_usage = 0
        heap_size = 0
        stack_size = 0
        boot_size = 0
        ram_usage = 0
        
        for section in elf.iter_sections():
            sh_size = section['sh_size']
            sh_flags = section['sh_flags']
            sh_type = section['sh_type']

            if sh_type == 'SHT_PROGBITS' and not sh_flags & 0x8:  # SHF_ALLOC flag
                memory_usage += sh_size

            if '.heap' in section.name:
                heap_size += sh_size

            elif '.stack' in section.name:
                stack_size += sh_size

            elif '.bss' in section.name:
                ram_usage += sh_size

            elif '.boot_code' in section.name:
                boot_size += sh_size

        print(f"Heap Memory: {round(heap_size/1024/1024, 3)} MB")
        print(f"Stack Memory: {round(stack_size)} bytes")
        print(f"Boot Code Size: {round(boot_size)} bytes")
        print(f"RAM Usage: {round(ram_usage/1024/1024, 3)} MB")

        memory_usage_kb = memory_usage/1024
        memory_percent = memory_usage_kb/256           
        print(f"ROM: [{'='*round(memory_percent*16)}{'.'*(16-round(memory_percent*16))}] {round(memory_percent*100, 1)}% (used {round(memory_usage_kb, 3)} kB of 256 kB)")

    return memory_usage

def main():
    parser = argparse.ArgumentParser(description='Calculate memory usage of an ELF file.')
    parser.add_argument('elf_file', type=str, help='Path to the ELF file')
    args = parser.parse_args()
    print_memory_usage(args.elf_file)

if __name__ == "__main__":
    main()
