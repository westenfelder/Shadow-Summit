# Risky-Science
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html
- https://secret.club/2023/12/24/riscy-business.html

## Setup
```bash
# Ubuntu 25.04
apt install golang qemu-user make nodejs npm autoconf clang -y

# Install riscv tools
npm install --location=global xpm@latest
xpm install @xpack-dev-tools/riscv-none-elf-gcc@15.2.0-1.1 --verbose --global --force
export PATH="/root/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc/15.2.0-1.1/.content/bin:$PATH"

# Install rvemu
# git clone https://github.com/ksco/rvemu
cd rvemu
make -j
```

## Go
```bash
GOOS=linux GOARCH=riscv64 go build -o add_go main.go
qemu-riscv64 ./add_go
```

## C
```bash
riscv-none-elf-gcc main.c -o add_c
qemu-riscv32 ./add_c
```
