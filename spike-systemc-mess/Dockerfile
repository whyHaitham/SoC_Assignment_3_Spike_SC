# Spike RISC-V ISS inside SystemC, running the official MESS traffic generator.
#
#   docker build -t systemc-riscv-mess:v2 .
#   docker run --rm -v "$PWD/results:/root/spike-systemc-mess/results" systemc-riscv-mess:v2
#
# Every upstream repository is pinned to the commit the project was tested with.
FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG SPIKE_COMMIT=118480991eb16c6a6f40b94fdc35acc9da2ceacd
ARG TOOLCHAIN_COMMIT=317ee3dd0f45156fbfbd112db9899e0933fb2911
ARG PK_COMMIT=9c61d29846d8521d9487a57739330f9682d5b542
# bsc-mem/Mess v2.1.0
ARG MESS_COMMIT=387248d7114be930019202518106ead0b3b304f6
ARG SYSTEMC_VERSION=2.3.3

ENV RISCV=/opt/riscv
ENV SYSTEMC_HOME=/usr/local/systemc-2.3.3-cxx17
ENV PATH=/opt/riscv/bin:${PATH}
ENV PKG_CONFIG_PATH=/opt/riscv/lib/pkgconfig

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake autotools-dev bc bison build-essential ca-certificates \
    cmake curl device-tree-compiler file flex gawk git gperf libboost-regex-dev \
    libboost-system-dev libexpat1-dev libfl-dev libgmp-dev libmpc-dev \
    libmpfr-dev libtool ninja-build patch patchutils pkg-config python3 \
    python3-matplotlib texinfo wget zlib1g-dev && \
    rm -rf /var/lib/apt/lists/*

# Shallow-fetch one exact commit of a repository: fetch_commit <url> <sha> <dir>
RUN printf '%s\n' '#!/bin/sh' 'set -eu' \
      'git init -q "$3" && cd "$3"' \
      'git remote add origin "$1"' \
      'git fetch -q --depth 1 origin "$2"' \
      'git checkout -q FETCH_HEAD' \
      'test "$(git rev-parse HEAD)" = "$2"' \
      > /usr/local/bin/fetch_commit && chmod +x /usr/local/bin/fetch_commit

# GNU RISC-V bare-metal compiler + newlib.  No --recursive: only the three
# submodules needed for a newlib toolchain are fetched, at their pinned SHAs.
RUN fetch_commit https://github.com/riscv-collab/riscv-gnu-toolchain.git \
      "${TOOLCHAIN_COMMIT}" /tmp/riscv-gnu-toolchain && \
    cd /tmp/riscv-gnu-toolchain && \
    git submodule update --init --depth 1 binutils gcc newlib && \
    ./configure --prefix=${RISCV} --with-arch=rv64gc --with-abi=lp64d --disable-gdb && \
    make -j"$(nproc)" newlib && \
    cd / && rm -rf /tmp/riscv-gnu-toolchain

# SystemC built with the same C++17 ABI as the wrapper.
RUN wget -q https://github.com/accellera-official/systemc/archive/refs/tags/${SYSTEMC_VERSION}.tar.gz -O /tmp/systemc.tar.gz && \
    mkdir -p /tmp/systemc-src /tmp/systemc-build && \
    tar -xzf /tmp/systemc.tar.gz -C /tmp/systemc-src --strip-components=1 && \
    cd /tmp/systemc-build && \
    cmake /tmp/systemc-src -DCMAKE_INSTALL_PREFIX=${SYSTEMC_HOME} \
      -DCMAKE_CXX_STANDARD=17 -DBUILD_SHARED_LIBS=ON && \
    cmake --build . -j"$(nproc)" && cmake --install . && \
    echo "${SYSTEMC_HOME}/lib" >/etc/ld.so.conf.d/systemc.conf && ldconfig && \
    rm -rf /tmp/systemc.tar.gz /tmp/systemc-src /tmp/systemc-build

# Spike as a library, with the SystemC stepping / miss callbacks and the
# fetch-trace length fix.
COPY patches/spike-systemc.patch /tmp/spike-systemc.patch
RUN fetch_commit https://github.com/riscv-software-src/riscv-isa-sim.git \
      "${SPIKE_COMMIT}" /root/riscv-src/riscv-isa-sim && \
    cd /root/riscv-src/riscv-isa-sim && \
    git submodule update --init --depth 1 berkeley-softfloat-3 && \
    git apply /tmp/spike-systemc.patch && \
    mkdir build && cd build && \
    ../configure --prefix=${RISCV} && \
    make -j"$(nproc)" && make install && ldconfig

# Proxy kernel that loads the newlib RISC-V workload.
RUN fetch_commit https://github.com/riscv-software-src/riscv-pk.git \
      "${PK_COMMIT}" /root/riscv-src/riscv-pk && \
    cd /root/riscv-src/riscv-pk && mkdir build && cd build && \
    ../configure --prefix=${RISCV} --host=riscv64-unknown-elf && \
    make -j"$(nproc)" && make install

# Official MESS benchmark sources (code generator + traffic_gen.c).
RUN fetch_commit https://github.com/bsc-mem/Mess.git \
      "${MESS_COMMIT}" /root/riscv-src/mess

COPY . /root/spike-systemc-mess
WORKDIR /root/spike-systemc-mess

RUN make all

# Plots and CSVs are written here; mount a host directory to collect them.
VOLUME /root/spike-systemc-mess/results

CMD ["make", "plot"]
