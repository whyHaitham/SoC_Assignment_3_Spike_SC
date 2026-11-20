// Spike RISC-V ISS embedded in a SystemC module.
//
// A 1 GHz sc_clock drives the model: on every rising clock edge Spike
// executes exactly one instruction.  Every cache miss detected by Spike's
// cache model inside the MESS region of interest (between the mess_roi_begin
// and mess_roi_end symbols) is logged with its SystemC time.
//
// Usage: systemc_spike_mess <roi-begin-hex> <roi-end-hex> <pk> <program> [args...]
// Output: cache_miss_log.csv in the current directory.

#include <systemc>

#include <riscv/cachesim.h>
#include <riscv/cfg.h>
#include <riscv/devices.h>
#include <riscv/mmu.h>
#include <riscv/platform.h>
#include <riscv/sim.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace sc_core;

SC_MODULE(SpikeSystemC)
{
    sc_clock clk;                          // 1 GHz CPU clock
    reg_t roi_begin, roi_end;
    std::vector<std::string> target_args;  // pk, program, program args

    SC_HAS_PROCESS(SpikeSystemC);

    SpikeSystemC(sc_module_name name, reg_t begin, reg_t end,
                 std::vector<std::string> args)
        : sc_module(name),
          clk("clk", sc_time(1, SC_NS)),
          roi_begin(begin), roi_end(end),
          target_args(std::move(args))
    {
        SC_THREAD(run_spike);
    }

    void run_spike()
    {
        constexpr reg_t ram_size = 256ULL * 1024ULL * 1024ULL;

        cfg_t cfg;
        cfg.mem_layout.clear();
        cfg.mem_layout.emplace_back(DRAM_BASE, ram_size);

        mem_t ram(ram_size);
        std::vector<std::pair<reg_t, abstract_mem_t*> > memories;
        memories.emplace_back(DRAM_BASE, &ram);

        std::vector<device_factory_sargs_t> devices;
        debug_module_config_t debug_config;

        // L1 caches: 64 sets x 4 ways x 64-B lines = 16 KiB each.  Declared
        // before Spike so that Spike's MMU is destroyed before its tracers.
        icache_sim_t icache("64:4:64", "I$");
        dcache_sim_t dcache("64:4:64", "D$");

        // Spike used as a C++ library object, not as the spike executable.
        sim_t spike(&cfg, false, memories, devices, false, target_args,
                    debug_config, nullptr, true, nullptr, false, nullptr,
                    std::nullopt);

        std::ofstream log("cache_miss_log.csv");
        if (!log)
            throw std::runtime_error("cannot create cache_miss_log.csv");
        log << "miss_index,time_ns,cache,access,address,size_bytes\n";

        bool roi_active = false;
        std::uint64_t misses = 0, instructions = 0;

        // Called by the patched cache_sim_t::access() whenever a miss is
        // detected: log the SystemC time of the miss.
        auto log_miss = [&](const std::string& cache, uint64_t address,
                            size_t bytes, bool store)
        {
            if (!roi_active)
                return;
            const std::uint64_t now_ns = sc_time_stamp().value() /
                                         sc_time(1, SC_NS).value();
            log << misses++ << ',' << now_ns << ','
                << (cache == "D$" ? "dcache" : "icache") << ','
                << (store ? "write" : "read") << ",0x"
                << std::hex << address << std::dec << ',' << bytes << '\n';
        };

        icache.set_miss_callback(log_miss);
        dcache.set_miss_callback(log_miss);
        spike.get_core(0)->get_mmu()->register_memtracer(&icache);
        spike.get_core(0)->get_mmu()->register_memtracer(&dcache);

        // Called by the patched sim_t::idle() after every single instruction,
        // on the stack of this SC_THREAD: wait for the next clock edge, so the
        // SystemC kernel triggers each instruction.
        spike.set_systemc_step_callback([&]()
        {
            ++instructions;
            const reg_t next_pc = spike.get_core(0)->get_state()->pc;
            if (next_pc == roi_begin && !roi_active) {
                roi_active = true;
                std::cout << "[" << sc_time_stamp() << "] entered MESS ROI\n";
            } else if (next_pc == roi_end && roi_active) {
                roi_active = false;
                std::cout << "[" << sc_time_stamp() << "] left MESS ROI\n";
            }
            wait(clk.posedge_event());
        });

        std::cout << "[" << sc_time_stamp() << "] starting Spike, clock "
                  << clk.period() << " (1 GHz)\n";
        wait(clk.posedge_event());
        const int status = spike.run();
        log.flush();

        std::cout << "[" << sc_time_stamp() << "] Spike exited with status "
                  << status << " after " << instructions << " instructions; "
                  << misses << " cache misses logged in the ROI\n";
        sc_stop();
    }
};

int sc_main(int argc, char* argv[])
{
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <roi-begin-hex> <roi-end-hex> <pk> <program> [args...]\n";
        return 1;
    }
    sc_set_time_resolution(1, SC_PS);
    try {
        const reg_t begin = std::stoull(argv[1], nullptr, 16);
        const reg_t end = std::stoull(argv[2], nullptr, 16);
        SpikeSystemC top("top", begin, end,
                         std::vector<std::string>(argv + 3, argv + argc));
        sc_start();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
