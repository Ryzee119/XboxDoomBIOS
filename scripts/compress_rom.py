from elftools.elf.elffile import ELFFile
import lz4.block
import struct
import sys

def compress_rom(input_file, output_file):
    original_data = bytearray()

    # Open the ELF file and extract the sections .text, .data, and .rodata and push into original_data array
    # to compress as one blob
    with open(input_file, 'rb') as f:
        elf = ELFFile(f)
        sections_to_extract = ['.text', '.data', '.rodata']
        for section in elf.iter_sections():
            if section.name in sections_to_extract:
                original_data.extend(section.data())

    # Compress the data using LZ4, size of the uncompressed data is stored in the header
    compressed_data = lz4.block.compress(original_data, mode='high_compression', store_size = True)

    with open(output_file, 'wb') as f_out:
        f_out.write(compressed_data)

if __name__ == "__main__":
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    compress_rom(input_file, output_file)
