asm(
    ".section .text\n"
    ".global _start\n"
    "_start:\n"
    "    jal main\n" // main()
    "    li a7, 94\n" // __NR_exit_group
    "    ecall\n"
);


int main() {
    const char *message = "Risky-Science\n";
    
    long len = 15;
    long fd = 1; // STDOUT
    
    long syscall_num = 64; // __NR_write

    register long a7 asm("a7") = syscall_num;
    register long a0 asm("a0") = fd;
    register long a1 asm("a1") = (long)message;
    register long a2 asm("a2") = len;

    asm volatile (
        "ecall"
        : "+r" (a0)
        : "r" (a1), "r" (a2), "r" (a7)
        : "memory"
    );

    return 0;
}