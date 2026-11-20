SYSTEMC_HOME := /usr/local/systemc-2.3.3-cxx17
RISCV ?= /opt/riscv
PKG_CONFIG_PATH := $(RISCV)/lib/pkgconfig

CXX := g++
RISCV_GCC := $(RISCV)/bin/riscv64-unknown-elf-gcc
RISCV_NM := $(RISCV)/bin/riscv64-unknown-elf-nm
PK := $(RISCV)/riscv64-unknown-elf/bin/pk

SPIKE_CFLAGS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --cflags riscv-riscv riscv-fesvr)
SPIKE_LIBS := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config --libs riscv-riscv riscv-fesvr)

# ---- Official MESS traffic generator (bsc-mem/Mess) ----------------------
MESS_SRC ?= /root/riscv-src/mess
MESS_GEN := build/mess_gen/generated_utils_multiseq_riscv.c
MESS_ELF := build/traffic_gen_multiseq.x
# MESS defaults are used for the read ratio (50 %) and pause (0).  The arrays
# are sized for simulation (8192 doubles each, 4x the L1 data cache) and the
# run is bounded to 4 passes.
MESS_ARGS := -a 8192 -n 4

.PHONY: all run plot clean

all: build/systemc_spike_mess $(MESS_ELF)

build:
	mkdir -p build

# SystemC program that contains Spike as a library.
build/systemc_spike_mess: src/systemc_spike_mess.cpp | build
	$(CXX) -std=c++17 -O2 $< \
	  -I$(SYSTEMC_HOME)/include $(SPIKE_CFLAGS) \
	  -L$(SYSTEMC_HOME)/lib -Wl,-rpath,$(SYSTEMC_HOME)/lib \
	  -lsystemc $(SPIKE_LIBS) -pthread -ldl -o $@

# MESS's code generator, built for the host and driven for a scalar RISC-V target.
build/mess_codegen/gen_riscv_kernel: tools/mess_codegen/gen_riscv_kernel.cpp tools/mess_codegen/CMakeLists.txt | build
	cmake -S tools/mess_codegen -B build/mess_codegen -DCMAKE_BUILD_TYPE=Release -DMESS_SRC=$(MESS_SRC)
	cmake --build build/mess_codegen -j"$$(nproc)" --target gen_riscv_kernel

$(MESS_GEN): build/mess_codegen/gen_riscv_kernel
	$< $(CURDIR)/build/mess_gen

# Upstream traffic_gen.c plus the bounded-iteration / ROI patch.
build/mess_src/traffic_gen.c: $(MESS_SRC)/src/traffic_gen/src/traffic_gen.c patches/mess-traffic_gen-pk.patch | build
	mkdir -p build/mess_src
	cp $(MESS_SRC)/src/traffic_gen/src/traffic_gen.c $(MESS_SRC)/src/traffic_gen/src/utils.h build/mess_src/
	patch -d build/mess_src -p4 < patches/mess-traffic_gen-pk.patch

# Same -O0 as MESS's own traffic_gen Makefile; no OpenMP/MPI under pk.
MESS_CFLAGS := -march=rv64gc -mabi=lp64d -O0 -g -Wall -Wno-unused-but-set-variable \
  -Ibenchmark/mess/pk_compat -Ibuild/mess_src \
  -D_POSIX_TIMERS=1 -D_POSIX_MONOTONIC_CLOCK=1 -DTrafficGen_ARRAY_SIZE=1048576

$(MESS_ELF): build/mess_src/traffic_gen.c $(MESS_GEN) benchmark/mess/pk_compat/pk_compat.c
	$(RISCV_GCC) $(MESS_CFLAGS) build/mess_src/traffic_gen.c \
	  $(MESS_GEN) build/mess_gen/nop.c benchmark/mess/pk_compat/pk_compat.c -o $@

# Run MESS on Spike inside SystemC at 1 GHz; writes results/cache_miss_log.csv.
run: all
	mkdir -p results
	@BEGIN=$$($(RISCV_NM) $(MESS_ELF) | awk '$$3=="mess_roi_begin" {print $$1}'); \
	END=$$($(RISCV_NM) $(MESS_ELF) | awk '$$3=="mess_roi_end" {print $$1}'); \
	echo "MESS ROI: 0x$$BEGIN -> 0x$$END"; \
	cd results && ../build/systemc_spike_mess $$BEGIN $$END $(PK) \
	  ../$(MESS_ELF) $(MESS_ARGS) 2>&1 | tee simulation_console.log

# Histogram of inter-cache-miss time.
plot: run
	python3 scripts/plot_histogram.py results/cache_miss_log.csv \
	  --output results/inter_cache_miss_histogram.png \
	  --title "MESS on Spike/SystemC at 1 GHz (default MESS settings: 50% reads / 50% writes, pause 0)"

# `results/` may be a Docker volume mount point: empty it, never remove it.
clean:
	rm -rf build
	mkdir -p results && find results -mindepth 1 -delete
