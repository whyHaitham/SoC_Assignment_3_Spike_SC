// Host-side driver for the official MESS code generator (bsc-mem/Mess).
//
// MESS's GenerateCode detects the *host* CPU and generates kernels for it.
// Our host is AArch64 (Docker on Apple Silicon) but the target is the RISC-V
// core simulated by Spike, so this driver calls the very same MESS classes
// (KernelGenerator -> RiscvArchitecture -> RiscvAssembler) with the
// capabilities of the simulated core: RV64GC, scalar (no RVV), 64-B lines.
//
// Output (identical in content to what GenerateCode would emit on a scalar
// RISC-V host):
//   <out>/generated_utils_multiseq_<arch>.c   TrafficGen_copy_rw() + ratios
//   <out>/nop.c                                 pause (nop) kernel
#include "Codegen.h"
#include "KernelTypes.h"
#include "SystemDetection.h"

#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <absolute-output-dir>\n";
        return 1;
    }
    const std::filesystem::path out = std::filesystem::absolute(argv[1]);

    CPUCapabilities caps{};
    caps.arch = CPUArchitecture::RISCV64;
    caps.model_name = "Spike RV64GC (scalar)";
    caps.physical_cores = caps.logical_cores = caps.sockets = 1;
    caps.cache_line_size_bytes = 64;
    caps.l1d_size = caps.l1i_size = 16 * 1024;   // matches the 64:4:64 caches

    KernelConfig config;            // MESS defaults (ratio granularity, ops,
    config.isa_mode = ISAMode::SCALAR;   // pause block, interleaving, ...)

    KernelGenerator generator(config, caps);
    const ArchitectureConfig arch = generator.setup_architecture_config();
    generator.generate_kernel(out.string());

    std::cout << "MESS kernel generated for architecture '" << arch.arch_name
              << "' (ISA SCALAR) in " << out << '\n';
    return 0;
}
