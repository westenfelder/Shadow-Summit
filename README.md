# Risky-Science
- https://secret.club/2023/12/24/riscy-business.html
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html

## Setup
```bash
# Ubuntu 25.04
apt install build-essential qemu-user make clang crossbuild-essential-riscv64 gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf -y

# Install rvemu
git clone https://github.com/ksco/rvemu
cd rvemu
make -j

# Compile
riscv64-linux-gnu-gcc -O0 -static -nostdlib -o test main.c

# Run
qemu-riscv64 ./test
./rvemu/rvemu ./test

# Disassemble
riscv64-linux-gnu-objdump -d ./test > test.dis
```
