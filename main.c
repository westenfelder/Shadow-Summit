#define __NR_read 63
#define __NR_write 64
#define __NR_exit_group 94
#define ALWAYS_INLINE inline __attribute__((always_inline))

typedef unsigned int uint32_t;

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
    const char *message = "Input: ";
    sys_write(1, message, 7);

    char key = 0x42;
    long flag_len = 20;

    char read_buffer[128];
    long bytes_read = sys_read(0, read_buffer, 128);

    // New Comparison: flag{Risky-Science}
    uint32_t target_digest = 3030167379; // digest of flag
    uint32_t input_digest = simple_hash(read_buffer,flag_len);
    
    print_int(input_digest);
    print_int(target_digest);

    if(sys_memcmp(&target_digest, &input_digest, 4) == 0) {
        const char *correct_msg = "Correct Hash\n";
        sys_write(1, correct_msg, 13);
    } else {
        const char *false_msg = "Incorrect Hash\n";
        sys_write(1, false_msg, 15);
    }

    return 0;
}
