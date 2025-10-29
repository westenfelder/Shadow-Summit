all:
	riscv64-linux-gnu-gcc -O0 -static -nostdlib -o crack main.c
	xxd -i crack > crack.h
	mv crack.h rvemu/src/crack.h
	cd rvemu && make -j
	mv rvemu/rvemu crack_x86
	mv crack crack_riscv
clean:
	cd rvemu && make clean
	rm -f rvemu/src/crack.h
	rm -f crack*