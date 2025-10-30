# Risky-Science
- https://secret.club/2023/12/24/riscy-business.html
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html

## Setup
```bash
# Ubuntu 25.04
apt install xxd build-essential qemu-user make clang crossbuild-essential-riscv64 gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf python3.12 python3-pip python3.12-venv gdb gdbserver nasm -y

# Build
make

# Run
./crack_x86

# Testing
qemu-riscv64 ./crack_riscv # should fail with illegal instruction
riscv64-linux-gnu-objdump -D ./crack_riscv > crack_riscv.dis
ndisasm -b 64 ./crack_x86 > crack_x86.dis

# Clean
make clean
```