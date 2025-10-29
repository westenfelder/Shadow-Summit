import sys
from elftools.elf.elffile import ELFFile

OPCODE_MAP = {
    0x37: "LUI",      # Load Upper Immediate
    0x17: "AUIPC",    # Add Upper Immediate to PC
    0x6F: "JAL",      # Jump and Link
    0x67: "JALR",     # Jump and Link Register
    0x63: "BRANCH",   # (BEQ, BNE, BLT, BGE, BLTU, BGEU)
    0x03: "LOAD",     # (LB, LH, LW, LBU, LHU, LWU)
    0x23: "STORE",    # (SB, SH, SW, SD)
    0x13: "OP-IMM",   # (ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI)
    0x33: "OP",       # (ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND)
    0x0F: "FENCE",    # (FENCE, FENCE.I)
    0x73: "SYSTEM",   # (ECALL, EBREAK, CSR*, MRET, SRET, WFI)
}

original_filename = 'crack_riscv'
new_filename = 'crack_riscv_swapped'

with open(original_filename, 'rb') as f:
    f.seek(0)
    original_file_data = bytearray(f.read())
    f.seek(0)
    elf = ELFFile(f)
    text_section = elf.get_section_by_name('.text')

    # Get the .text data, base address, and file offset
    data = text_section.data()
    base_addr = text_section['sh_addr']
    text_file_offset = text_section['sh_offset']
    text_size = text_section['sh_size']

    new_text_data = bytearray()
    offset = 0

    while offset < len(data):
        instruction_bytes = data[offset : offset + 4]

        # Convert to little-endian integer
        inst_word = int.from_bytes(instruction_bytes, 'little')

        # Get opcode (bits 6:0)
        opcode = inst_word & 0x7F
        current_addr = base_addr + offset
            
        new_inst_word = inst_word

        # Swap
        if opcode == 0x03: # LOAD
            new_inst_word = (inst_word & ~0x7F) | 0x13
        elif opcode == 0x13: # OP-IMM
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
            