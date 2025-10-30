import sys
import random
from elftools.elf.elffile import ELFFile

from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Random import get_random_bytes
import os

random.seed()

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

with open("rvemu/src/shuffle.h", "w") as f:
    f.write(c_header_content)


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

        for i in range(19):
            if opcode == OPCODE_MAP[i][0] and last_two_bits == 0x3:
                new_inst_word = (inst_word & ~0x7C) | (OPCODE_MAP[shuffle[i]][0] << 2)
                instructions_swapped+=1
                break
            
        new_text_data.extend(new_inst_word.to_bytes(4, 'little'))
            
        offset += 4

    # Replace .text section
    if len(new_text_data) != text_size:
        print("Fatal Error")
        sys.exit(1)
        
    start = text_file_offset
    end = text_file_offset + text_size
    original_file_data[start:end] = new_text_data

    # Write new ELF file
    with open(new_filename, 'wb') as new_f:
        new_f.write(original_file_data)


## AES Encryption (should be function)
key = b'my_super_secret_key_1234567890!!' # todo - change this
encrypted_filename = new_filename + ".enc"
try:
    with open(new_filename, 'rb') as f_in:
        plaintext_data = f_in.read()

    cipher = AES.new(key, AES.MODE_CBC)
    
    encrypted_data = cipher.encrypt(pad(plaintext_data, AES.block_size))
    with open(encrypted_filename, 'wb') as f_out:
        f_out.write(cipher.iv)         # Write the 16-byte IV first
        f_out.write(encrypted_data) # write the ciphertext
        
    print(f"Successfully encrypted '{new_filename}' to '{encrypted_filename}'")
    # To decrypt, you would read the first 16 bytes (IV),
    # then decrypt the rest using AES.new(key, AES.MODE_CBC, iv=iv)

except FileNotFoundError:
    print(f"Error: Could not find file '{new_filename}' to encrypt")
except Exception as e:
    print(f"An error occurred during encryption: {e}")

