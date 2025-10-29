import sys
from elftools.elf.elffile import ELFFile

OPCODE_MAP = {
    0x00: "LOAD",
    0x01: "LOAD-FP",
    0x03: "FENCE",
    0x04: "OP-IMM",
    0x05: "AUIPC",
    0x06: "OP-IMM-32",
    0x08: "STORE",
    0x09: "STORE-FP",
    0x0C: "OP",
    0x0D: "LUI",
    0x0E: "OP-32",
    0x10: "MADD",
    0x11: "MSUB",
    0x12: "NMSUB",
    0x13: "NMADD",
    0x14: "OP-FP",
    0x18: "BRANCH",
    0x19: "JALR",
    0x1B: "JAL",
    0x1C: "SYSTEM",
}

original_filename = 'crack'
new_filename = 'crack'

with open(original_filename, 'rb') as f:
    f.seek(0)
    original_file_data = bytearray(f.read())
    f.seek(0)
    elf = ELFFile(f)
    text_section = elf.get_section_by_name('.text')

    # Get the .text data and offsets
    data = text_section.data()
    base_addr = text_section['sh_addr']
    text_file_offset = text_section['sh_offset']
    text_size = text_section['sh_size']

    new_text_data = bytearray()
    offset = 0
    instructions_swapped = 0

    while offset < len(data):
            
        instruction_bytes = data[offset : offset + 4]

        # little-endian
        inst_word = int.from_bytes(instruction_bytes, 'little')

        # Get opcode (bits 6-2)
        opcode = (inst_word & 0x7C) >> 2
        last_two_bits = inst_word & 0x03
        current_addr = base_addr + offset
        
        new_inst_word = inst_word

        if opcode == 0x00 and last_two_bits == 0x3:
            new_inst_word = (inst_word & ~0x7C) | (0x02 << 2)
            instructions_swapped += 1

            print(f"{inst_word:032b}")
            print(f"{new_inst_word:032b}")
            print()
            
        new_text_data.extend(new_inst_word.to_bytes(4, 'little'))
            
        offset += 4

    # Replace .text section
    if len(new_text_data) != text_size:
        print("Error")
        sys.exit(1)
        
    start = text_file_offset
    end = text_file_offset + text_size
    original_file_data[start:end] = new_text_data

    # Write new ELF file
    with open(new_filename, 'wb') as new_f:
        new_f.write(original_file_data)
        
    print(f"Swapped {instructions_swapped} instructions.")