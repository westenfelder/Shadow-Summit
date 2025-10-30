.PHONY: all clean

# pick a RISC-V cross-compiler: prefer riscv64-unknown-elf-gcc, else riscv64-elf-gcc
ifeq ($(shell command -v riscv64-unknown-elf-gcc >/dev/null 2>&1 && echo yes),yes)
CC := riscv64-unknown-elf-gcc
else ifeq ($(shell command -v riscv64-elf-gcc >/dev/null 2>&1 && echo yes),yes)
CC := riscv64-elf-gcc
else
$(error Neither "riscv64-unknown-elf-gcc" nor "riscv64-elf-gcc" found. Install a RISC-V cross-compiler.)
endif

CFLAGS := -march=rv64g -O0 -static -nostdlib -s

all:
	if [ ! -e "crack" ]; then $(CC) $(CFLAGS) -o crack main.c; fi
	python3 -m venv riscv-venv && \
	riscv-venv/bin/pip install -r requirements.txt && \
	riscv-venv/bin/python3 main.py
	mv crack crack.orig && cp crack.enc crack
	xxd -i crack > crack.h
	mv crack.h rvemu/src/crack.h
	cd rvemu && make -j
	mv rvemu/rvemu crack_x86
	mv crack crack_riscv
clean:
	cd rvemu && make clean
	rm -f rvemu/src/crack.h
	rm -f rvemu/src/shuffle.h
	rm -f crack*
