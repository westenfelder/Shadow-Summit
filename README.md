# Risky-Science
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html
- https://secret.club/2023/12/24/riscy-business.html

```bash
# Ubuntu 25.04
git clone https://github.com/riscv/riscv-gnu-toolchain --recursive
apt install autoconf automake autotools-dev curl python3 python3-pip python3-tomli libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git make cmake libglib2.0-dev libslirp-dev golang qemu-user -y
```

```bash
GOOS=linux GOARCH=riscv64 go build -o hello-riscv main.go
qemu-riscv64 ./hello-riscv
```