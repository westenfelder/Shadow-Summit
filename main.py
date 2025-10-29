import sys
from elftools.elf.elffile import ELFFile

OPCODE_MAP = {
    0x0D: "LUI",      # 0x37
    0x05: "AUIPC",    # 0x17
    0x1B: "JAL",      # 0x6F
    0x19: "JALR",     # 0x67
    0x18: "BRANCH",   # 0x63
    0x00: "LOAD",     # 0x03
    0x08: "STORE",    # 0x23
    0x04: "OP-IMM",   # 0x13
    0x0C: "OP",       # 0x33
    0x03: "FENCE",    # 0x0F
    0x1C: "SYSTEM",   # 0x73
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

    while offset < len(data):
        instruction_bytes = data[offset : offset + 4]

        # little-endian
        inst_word = int.from_bytes(instruction_bytes, 'little')

        # Get opcode
        opcode = (inst_word & 0x7C) >> 2
        current_addr = base_addr + offset
            
        new_inst_word = inst_word

        # Swap
        if opcode == 0x00D:  # LOAD
            new_inst_word = (inst_word & ~0x7F) | 0x13
        elif opcode == 0x04:  # OP-IMM
            new_inst_word = (inst_word & ~0x7F) | 0x03
            
        new_text_data.extend(new_inst_word.to_bytes(4, 'little'))
            
        offset += 4

    # Replace .text section
    if len(new_text_data) != text_size:
        print("ERROR")
        sys.exit(1)
    start = text_file_offset
    end = text_file_offset + text_size
    original_file_data[start:end] = new_text_data

    # Write new ELF file
    with open(new_filename, 'wb') as new_f:
        new_f.write(original_file_data)
