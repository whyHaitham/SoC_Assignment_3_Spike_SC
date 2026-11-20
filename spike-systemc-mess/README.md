# Spike + SystemC + MESS: reproducing the inter-cache-miss histogram

## Requirements

- Docker
- About 20 GB of free Docker disk space during the build (the final image is
  about 3.3 GB)
- Internet access during the build (the pinned toolchain, SystemC, Spike, pk
  and MESS sources are downloaded)

## 1. Build the image

From this folder (the one that contains the `Dockerfile`):

```bash
docker build -t systemc-riscv-mess:v2 .
```

The build compiles the RISC-V GNU toolchain from source and takes roughly
1–3 hours.

## 2. Run the simulation and create the plot

```bash
mkdir -p results
docker run --rm -v "$PWD/results:/root/spike-systemc-mess/results" systemc-riscv-mess:v2
```

The container runs `make plot`, which:

1. runs the MESS benchmark on Spike inside the SystemC module at 1 GHz, and
2. plots the histogram of inter-cache-miss time.

## 3. Output

The files appear in `./results`:

| File | Contents |
|---|---|
| `inter_cache_miss_histogram.png` | histogram of the time between consecutive cache misses |
| `cache_miss_log.csv` | log of every cache miss with its simulation time in ns |
| `simulation_console.log` | console output of the simulation |

## Running the steps manually (optional)

Open a shell in the image:

```bash
docker run -it --rm -v "$PWD/results:/root/spike-systemc-mess/results" systemc-riscv-mess:v2 bash
```

Then, inside the container (`/root/spike-systemc-mess`):

```bash
make all     # build the SystemC program and the MESS RISC-V binary
make run     # run the simulation; writes results/cache_miss_log.csv
make plot    # run the simulation and plot results/inter_cache_miss_histogram.png
make clean   # remove build files and empty results/
```

## Using a pre-built image file (optional)

If the image was provided as `systemc-riscv-mess-v2.tar.gz`, load it instead
of building, then run step 2:

```bash
docker load -i systemc-riscv-mess-v2.tar.gz
```
