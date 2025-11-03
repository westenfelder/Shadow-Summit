#define __NR_read 63
#define __NR_write 64
#define __NR_exit_group 94
#define ALWAYS_INLINE inline __attribute__((always_inline))

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

asm(
    ".section .text\n"
    ".global _start\n"
    "_start:\n"
    "    jal main\n"
    "    li a7, 94\n"
    "    ecall\n"
);

long sys_write(long fd, const void *buf, long len) {
    register long a7 asm("a7") = __NR_write;
    register long a0 asm("a0") = fd;
    register long a1 asm("a1") = (long)buf;
    register long a2 asm("a2") = len;

    asm volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a7)
        : "memory"
    );

    return a0;
}

long sys_read(long fd, void *buf, long len) {
    register long a7 asm("a7") = __NR_read;
    register long a0 asm("a0") = fd;
    register long a1 asm("a1") = (long)buf;
    register long a2 asm("a2") = len;

    asm volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a7)
        : "memory"
    );

    return a0;
}

long sys_memcmp(const void *s1, const void *s2, long n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (long i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

void xor_string(char *data, long len, char key) {
    for (long i = 0; i < len; i++) {
        data[i] = data[i] ^ key;
    }
}

//churn function, rotat, xor, rotate xor 
#include <stdint.h>
#include <stddef.h>

// Rotate left and right for a byte
static inline uint8_t rol8(uint8_t val, int r_bits) {
    r_bits %= 8;
    return (val << r_bits) | (val >> (8 - r_bits));
}

static inline uint8_t ror8(uint8_t val, int r_bits) {
    r_bits %= 8;
    return (val >> r_bits) | (val << (8 - r_bits));
}

// 
void churn(uint8_t *data, int len, uint8_t key1, uint8_t key2) {
    for (int i = 0; i < len; ++i) {
        // Step 1: rotate right by 13 (mod 8)
        data[i] = ror8(data[i], 13);
        data[i] ^= key1;

        // rotate left by 7
        data[i] = rol8(data[i], 7);
        data[i] ^= key2;
    }
}


ALWAYS_INLINE static uint32_t simple_hash(char *data, int len)
{
    const unsigned char *bytes = (const unsigned char *)data;
    uint32_t hash = 0x9747b28c; // arbitrary seed (can change)
    uint32_t prime = 0x9e3779b1; // large prime for mixing

    for (int i = 0; i < len; ++i) {
        hash ^= bytes[i];
        hash *= prime;
        hash ^= hash >> 15;
    }

    // final avalanche
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;

    return hash;
}

void print_int(uint32_t input){
    char buffer[32] = {0};
    int i = 30;
    buffer[31] = '\n'; // newline at the end
    buffer[30] = '\0';

     while (input > 0 && i > 0) {
        buffer[i--] = '0' + (input % 10);
        input /= 10;
    }
    sys_write(1, &buffer[i + 1], 11);

}

int main() {
    long flag_len = 19;
    uint8_t key_1 = 0x67;
    uint8_t key_2 = 0x21;
    uint8_t encoded_flag[] = { 0x0b, 0x23, 0x17, 0x0f, 0x7f, 0xdb, 0x37, 0x5f, 0x3f, 0x77, 0x26, 0xdf, 0x1f, 0x37, 0x07, 0x2b, 0x1f, 0x07, 0x67 };

    const char *message = "Input: ";
    sys_write(1, message, 7);

    char read_buffer[128];
    long bytes_read = sys_read(0, read_buffer, 128);
    churn(read_buffer, flag_len, key_1, key_2);

    uint32_t orig_flag = simple_hash(read_buffer, flag_len);
    uint32_t py_flag = simple_hash(encoded_flag,flag_len);
    print_int(orig_flag);
    print_int(py_flag);

    if(sys_memcmp(encoded_flag, read_buffer, flag_len) == 0) {
        const char *correct_msg = "Correct Flag\n";
        sys_write(1, correct_msg, 13);
    } else {
        const char *false_msg = "Incorrect Flag\n";
        sys_write(1, false_msg, 15);
    }

    return 0;
}
