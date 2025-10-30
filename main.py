import sys
import random
from elftools.elf.elffile import ELFFile


OPCODE_MAP = {
    0: [0x00, "LOAD"],
    1: [0x01, "LOAD-FP"],
    2: [0x03, "FENCE"],
    3: [0x04, "OP-IMM"],
    4: [0x05, "AUIPC"],
    5: [0x06, "OP-IMM-32"],
    6: [0x08, "STORE"],
    7: [0x09, "STORE-FP"],
    8: [0x0C, "OP"],
    9: [0x0D, "LUI"],
    10: [0x0E, "OP-32"],
    11: [0x10, "MADD"],
    12: [0x11, "MSUB"],
    13: [0x12, "NMSUB"],
    14: [0x13, "NMADD"],
    15: [0x14, "OP-FP"],
    16: [0x18, "BRANCH"],
    17: [0x19, "JALR"],
    18: [0x1B, "JAL"],
}


# Shuffle opcode map indices
random.seed()
shuffle = list(range(19))
random.shuffle(shuffle)

c_array_body = ",\n".join([
    "    " + ", ".join(map(str, shuffle[i:i + 10]))
    for i in range(0, len(shuffle), 10)
])

c_header_content = f"""#ifndef SHUFFLE_H
#define SHUFFLE_H

static const int shuffle[{len(shuffle)}] = {{
{c_array_body}
}};

#endif // SHUFFLE_H
"""

# Write to header
with open("rvemu/src/shuffle.h", "w") as f:
    f.write(c_header_content)


# Patch binary
with open('crack', 'rb') as f:

    # Get the .text data and offsets
    f.seek(0)
    original_file_data = bytearray(f.read())
    f.seek(0)
    elf = ELFFile(f)
    text_section = elf.get_section_by_name('.text')
    data = text_section.data()
    base_addr = text_section['sh_addr']
    text_file_offset = text_section['sh_offset']
    text_size = text_section['sh_size']

    new_text_data = bytearray()
    offset = 0

    while offset < len(data):
            
        instruction_bytes = data[offset : offset + 4]

        # Convert to little-endian
        inst_word = int.from_bytes(instruction_bytes, 'little')
        current_addr = base_addr + offset

        # Get opcode (bits 6-2)
        opcode = (inst_word & 0x7C) >> 2
        # check for compression
        last_two_bits = inst_word & 0x03
        new_inst_word = inst_word

        # Shuffle
        for i in range(19):
            if opcode == OPCODE_MAP[i][0] and last_two_bits == 0x3:
                new_inst_word = (inst_word & ~0x7C) | (OPCODE_MAP[shuffle[i]][0] << 2)
                break
            
        new_text_data.extend(new_inst_word.to_bytes(4, 'little'))
        offset += 4

    # Update .text section
    if len(new_text_data) != text_size:
        print("Fatal Error")
        sys.exit(1)

    start = text_file_offset
    end = text_file_offset + text_size
    original_file_data[start:end] = new_text_data

    # Write new ELF file
    with open('crack', 'wb') as new_f:
        new_f.write(original_file_data)
