# Risky-Science
- https://secret.club/2023/12/24/riscy-business.html
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html

## Setup
```bash
# Ubuntu 25.04
apt install xxd build-essential qemu-user make clang crossbuild-essential-riscv64 gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf -y

# Build
make

# Run
# flag{Risky-Science}
qemu-riscv64 ./crack_riscv
./crack_x86

# Disassemble
riscv64-linux-gnu-objdump -D ./crack_riscv > crack_riscv.dis
ndisasm -b 64 ./crack_x86 > crack_x86.dis

# Clean
make clean
rm crack_riscv.dis crack_x86.dis
```
