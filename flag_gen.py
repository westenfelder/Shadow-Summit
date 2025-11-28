# Use this to generated the encoded flag that is churned, paste the output array and length into main.c

def rol(val, r_bits, max_bits=8):
    """Rotate left (circular shift) a byte."""
    return ((val << r_bits) & (2**max_bits - 1)) | (val >> (max_bits - r_bits))

def ror(val, r_bits, max_bits=8):
    """Rotate right (circular shift) a byte."""
    return ((val >> r_bits) | (val << (max_bits - r_bits))) & (2**max_bits - 1)

def encode_string(s, key1, key2):
    result = []
    for b in s.encode():
        # rotate right by 13 (mod 8 to rotate within a byte)
        b1 = ror(b, 13 % 8)  
        b2 = b1 ^ key1

        # rotate left by 7
        b3 = rol(b2, 7 % 8)
        b4 = b3 ^ key2

        result.append(b4)
    
    return result

def to_c_byte_array(byte_list, var_name="encoded_flag"):
    hex_values = ', '.join(f'0x{b:02x}' for b in byte_list)
    return f"unsigned char {var_name}[] = {{ {hex_values} }};"

input_string = "flag{Risky-Science}"
key1 = 0x67
key2 = 0x21

encoded = encode_string(input_string, key1, key2)
c_array_str = to_c_byte_array(encoded)
print(c_array_str)
print(f'flag_len={len(input_string)};')
