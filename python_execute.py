import argparse
import os
import shutil
import sys
import subprocess
import random
from elftools.elf.elffile import ELFFile
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
from Crypto.Random import get_random_bytes
import os

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


bytes_written = b''

def shuffle():
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
    with open('crack_riscv', 'rb') as f:

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
        with open('crack_riscv_patch', 'wb') as new_f:
            new_f.write(original_file_data)

def encryption(key):
    """Encrypts the file via AES"""
    ## AES Encryption
    # key = b'my_super_secret_key_1234567890!!'
    with open('crack_riscv_patch', 'rb') as f_in:
        plaintext_data = f_in.read()

    cipher = AES.new(key, AES.MODE_CBC)

    encrypted_data = cipher.encrypt(pad(plaintext_data, AES.block_size))
    with open('crack_riscv_enc', 'wb') as f_out:
        f_out.write(cipher.iv) # 16-byte IV
        f_out.write(encrypted_data) # ciphertext
        bytes_written = cipher.iv + encrypted_data
        return bytes_written


INVALID_FILETYPE = "Err : Invalid File Format, RISCY SCIENCE Utilizes .elf files as a precursor. Must be a .elf file"
INVALID_PATH = "Err : Invalid File Path"

def xxd_implementation(data : bytes, key):
    """Creates a hexdump the same way xxd would, hides a bit more what is being done since xxd is not being executed"""
    try:
        with open('crack.h', 'w') as f:

            f.write("unsigned char crack_riscv_enc[] = {\n")
            hex_values = ([hex(i) for i in data])
            result = ','.join(hex_values)
            f.write(result)
            f.write("\n")
            f.write("};")
            f.write("\n unsigned int crack_riscv_enc_len = ")
            f.write(str(len(data))+ ";" + "\n")
            f.write("unsigned char __key[] = {\n")
            hex_key_val = ([hex(i) for i in key])
            result_key = ','.join(hex_key_val)
            f.write(result_key + "};")

    except Exception as e:
        print(f"ERR : {e}")
        

def is_elf(file_name):
    """
    Validates file is a .elf file
    """
    ELF_MAGIC = b'\x7fELF'

    try: 
        with open(file_name, 'rb') as f:
            magic_byte = f.read(4)
            return magic_byte == ELF_MAGIC
    except IOError as e:

        return False
    
def validate_file_path(file_path):
    """
    Validates file path is valid
    """
    if os.path.exists(file_path):
        return True
    else:
        print(INVALID_PATH)
        return False


ascii_art = """  ______  _                      ______       _                         
                (_____ \(_)                    / _____)     (_)                        
                _____) )_  ___  ____ _   _   ( (____   ____ _ _____ ____   ____ _____ 
                |  __  /| |/___)/ ___) | | |   \____ \ / ___) | ___ |  _ \ / ___) ___ |
                | |  \ \| |___ ( (___| |_| |   _____) | (___| | ____| | | ( (___| ____|
                |_|   |_|_(___/ \____)\__  |  (______/ \____)_|_____)_| |_|\____)_____)
                                     (____/                                            """

def main():
    try:
        print(ascii_art)
        parser = argparse.ArgumentParser(description = 'Takes in a .elf file and does some RISCY SCIENCE!')
        parser.add_argument('filename', type = str, help = 'The name of the .elf file')
        args = parser.parse_args()
        file_name = args.filename
        # TODO: Allow User to designate build directory for emulator
        build_dir  = 'rvemu'
        source_file = os.path.join(build_dir, 'rvemu')
        # TODO: Allow User To Specify Filename
        dest_file = 'crack_x86'

        if is_elf(file_name) and validate_file_path(os.getcwd()):
            print("Valid File! Proceeding")
            # Random 32 byte key for now TODO: Allow password encryption for access to key
            key_random_128 = os.urandom(32)
            shuffle()
            hex_dump = encryption(key_random_128)
            print(hex_dump)
            xxd_implementation(hex_dump, key_random_128)
            shutil.move("crack.h", "rvemu/src/crack.h")
            # emu has a specific path that builds and compiles
            subprocess.run(['make', '-j'], cwd=build_dir, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

            # Move files to necessitated positions (also easier for user to find path)
            shutil.move(source_file, dest_file)

            # Clean up artifacts
            os.remove("crack_riscv_enc")
            os.remove("crack_riscv_patch")
        else:
            print(INVALID_FILETYPE)
    except (OSError, subprocess.CalledProcessError)  as e:
        print(e)

if __name__ == '__main__':
    main()