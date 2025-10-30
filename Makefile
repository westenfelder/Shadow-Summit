all:
	if [ ! -e "crack" ]; then riscv64-linux-gnu-gcc -s -march=rv64g -O0 -static -nostdlib -o crack main.c; fi
	python3 -m venv riscv-venv && \
	riscv-venv/bin/pip install -r requirements.txt && \
	riscv-venv/bin/python3 main.py
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