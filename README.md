# Risky-Science
- https://fmash16.github.io/content/posts/riscv-emulator-in-c.html
- https://secret.club/2023/12/24/riscy-business.html

```bash
# Ubuntu 25.04
apt install golang qemu-user make nodejs npm autoconf -y
npm install --location=global xpm@latest
xpm install @xpack-dev-tools/riscv-none-elf-gcc@15.2.0-1.1 --verbose --global --force
export PATH="/root/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc/15.2.0-1.1/.content/bin:$PATH"
```


```bash
GOOS=linux GOARCH=riscv64 go build -o hello-riscv main.go
qemu-riscv64 ./hello-riscv
```