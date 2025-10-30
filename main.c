#define __NR_read 63
#define __NR_write 64
#define __NR_exit_group 94

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

int main() {
    const char *message = "Input: ";
    sys_write(1, message, 7);

    // flag{Risky-Science}
    char encoded_flag[] = {
        0x24, 0x2e, 0x23, 0x25, 0x39, 0x10, 0x2b, 0x31, 0x29, 0x3b,
        0x6f, 0x11, 0x21, 0x2b, 0x27, 0x2c, 0x21, 0x27, 0x3f, 0x48
    };
    
    char key = 0x42;
    long flag_len = 20;

    char read_buffer[128];
    long bytes_read = sys_read(0, read_buffer, 128);

    xor_string(encoded_flag, flag_len, key);

    if (bytes_read == flag_len && sys_memcmp(read_buffer, encoded_flag, flag_len) == 0) {
        const char *correct_msg = "Correct\n";
        sys_write(1, correct_msg, 8);
    } else {
        const char *false_msg = "Incorrect\n";
        sys_write(1, false_msg, 10);
    }

    return 0;
}