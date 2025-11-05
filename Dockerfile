# Use an Ubuntu base image (you could pick Debian or other if preferred)
FROM ubuntu:24.04

# Install build dependencies for the toolchain (per project README)  
RUN apt-get update && apt-get install -y \
    qemu-system qemu-user qemu-utils wget llvm \
    autoconf automake autotools-dev curl python3 python3-pip python3-tomli \
    libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo \
    gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev \
 && rm -rf /var/lib/apt/lists/*

# Set environment variables
ENV RISCV=/opt/riscv
ENV PATH=${RISCV}/bin:${PATH}
ENV TOOL_LOC=/root/riscv-gnu-toolchain
ENV TEST_LOC=/root/riscv-tests

# Clone repos
RUN git clone https://github.com/riscv-collab/riscv-gnu-toolchain.git --recursive ${TOOL_LOC}
RUN git clone https://github.com/riscv-software-src/riscv-tests ${TEST_LOC}

# Build Toolchain
WORKDIR ${TOOL_LOC}
RUN ./configure --prefix=${RISCV} --enable-multilib \
 && make -j$(nproc)

# Build Tools
WORKDIR ${TEST_LOC}
RUN git submodule update --init --recursive
RUN autoconf
RUN ./configure --prefix=$RISCV/target 
RUN make
RUN make install
#RUN wget -O test_build.py https://raw.githubusercontent.com/fmash16/riscv_emulator/refs/heads/main/test.py

# Clean up build artifacts to reduce image size
RUN rm -rf ${TOOL_LOC}
#RUN rm -f ${TEST_LOC}/test_build.py

WORKDIR /root

# show version info when complete
CMD ["riscv64-unknown-elf-gcc", "--version"]
