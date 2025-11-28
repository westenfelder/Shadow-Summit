# Risky-Science
- https://secret.club/2023/12/24/riscy-business.html
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html

## Setup
### Docker
`docker pull pr1or1tyq/riscv_env:1.0`
`docker run --mount type=bind,src=</path/to/repo>,dst=/root/proj -it pr1or1tyq/riscv_env:1.0 /bin/bash`

```bash
# Ubuntu 25.04
apt install xxd build-essential qemu-user make clang crossbuild-essential-riscv64 gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf python3.12 python3-pip python3.12-venv gdb gdbserver nasm -y

# Build
make

# Run
./crack_x86

# Testing
chmod 777 crack_riscv*
qemu-riscv64 ./crack_riscv
qemu-riscv64 ./crack_riscv_patch # should fail with illegal instruction
qemu-riscv64 ./crack_riscv_enc # should fail with exec format error

# Disassemble
riscv64-linux-gnu-objdump -D ./crack_riscv > crack_riscv.dis
ndisasm -b 64 ./crack_x86 > crack_x86.dis

# Clean
make clean
```
