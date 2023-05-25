#include <cstdio>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <vector>
#include <chrono>

double sc_time_stamp() { return 0; }

#include "itype_mem.h"
#include "jmps_mem.h"
#include "dhrystone_mem.h"
#include <verilated_vcd_c.h>
#include "VCore__Syms.h"

#define MEMSIZE 8192*8
#define MEMBASE 0x100000

typedef struct {
    uint32_t idx;
    uint32_t value;
} val_t;

typedef struct {
    std::vector<val_t> regs;
    std::vector<val_t> mem;
} check_t;

static uint32_t be2mask(uint8_t be) {
    uint32_t mask = 0;
    for (unsigned i = 0; i < 4; i++) {
        if (be & (1 << i)) {
            mask |= 0xff << i*8;
        }
    }
    return mask;
}

static unsigned addr2idx(uint32_t addr, size_t mem_base) {
    return (addr-mem_base) / sizeof(uint32_t);
}

static bool finished = false;

static void clock(VCore &core) {
    core.clock = 0;
    core.eval();
    core.clock = 1;
    core.eval();
}

static float simulate(VCore &core, VerilatedVcdC *trace, uint32_t* mem, size_t len, size_t mem_base, unsigned max_cycles, bool verbose) {
    uint64_t cycle = 0;
    float simFrequency = 0;
    core.reset = 1;
    clock(core);
#ifdef TRACE
    trace->dump(cycle);
#endif
    core.reset = 0;
    core.eval();

    uint32_t next_imem_rdata = 0;
    uint8_t next_imem_rvalid = 0;
    uint32_t next_dmem_rdata = 0;
    uint8_t next_dmem_rvalid = 0;

    ++cycle;
    auto duration = std::chrono::high_resolution_clock::duration::zero();

    for (unsigned i = 0; i < max_cycles; i++) {
        core.io_imem_rvalid = next_imem_rvalid;
        core.io_imem_rdata = next_imem_rdata;
        core.io_dmem_rvalid = next_dmem_rvalid;
        core.io_dmem_rdata = next_dmem_rdata;

        next_imem_rvalid = core.io_imem_req;
        next_dmem_rvalid = core.io_dmem_req;
        core.io_dmem_gnt = core.io_dmem_req;

        auto t0 = std::chrono::high_resolution_clock::now();
        core.eval();
        auto t1 = std::chrono::high_resolution_clock::now();
        duration += t1 - t0;

        if (core.io_imem_req) {
            if (verbose) {
                printf("reading instruction at 0x%x: ", core.io_imem_addr);
                // Flush to get it out before the assert below.
                fflush(stdout);
            }
            auto memidx = addr2idx(core.io_imem_addr, mem_base);
            assert(memidx < len);
            next_imem_rdata = mem[memidx];
            if (verbose) printf("0x%x\n", next_imem_rdata);
        }

        if (core.io_dmem_req && core.io_dmem_we) {
            uint32_t write = core.io_dmem_wdata;
            uint32_t mask = be2mask(core.io_dmem_be);
            auto memidx = addr2idx(core.io_dmem_addr, mem_base);
            if (verbose) {
                printf("writing 0x%x to 0x%x (with mask 0x%x)\n", write & mask, core.io_dmem_addr, mask);
                // Flush to get it out before the assert below.
                fflush(stdout);
            }
            if (core.io_dmem_addr == 0xffff00) {
                finished = true;
                fprintf(stderr, "** dhrystones per second: %u **\n", write & mask);
                fprintf(stderr, "** execution finished at cycle %u **\n", i);
                break;
            } else {
                assert(memidx < len);
                mem[memidx] = (write & mask) | (~mask & mem[memidx]);
            }
        } else if (core.io_dmem_req) {
            auto memidx = addr2idx(core.io_dmem_addr, mem_base);
            if (verbose) {
                printf("reading data at 0x%x: ", core.io_dmem_addr);
                // Flush to get it out before the assert below.
                fflush(stdout);
            }
            assert(memidx < len);
            next_dmem_rdata = mem[memidx];
            if (verbose) printf("0x%x\n", next_dmem_rdata);
        }

        t0 = std::chrono::high_resolution_clock::now();
        clock(core);
        t1 = std::chrono::high_resolution_clock::now();
        duration += t1 - t0;
#ifdef TRACE
        trace->dump(cycle);
#endif
        ++cycle;

        auto seconds =
            std::chrono::duration_cast<std::chrono::duration<double>>(duration)
            .count();
        simFrequency = i / seconds;
        if (i % 10000 == 0 && i != 0)
            std::cerr << "Cycle " << i << " [" << simFrequency << " Hz]\n";
    }

    return simFrequency;
}

static bool check_results(VCore &core, uint32_t* mem, size_t mem_base, check_t* check) {
    for (auto v : check->regs) {
        auto got = core.rootp->Core__DOT__rf__DOT__regs_ext__DOT__Memory[v.idx];
        auto expected = v.value;
        if (got != expected) {
            printf("FAIL: regs[%d]: %d != %d\n", v.idx, got, expected);
            return true;
        }
    }
    for (auto v : check->mem) {
        auto got = mem[addr2idx(v.idx, mem_base)];
        auto expected = v.value;
        if (got != expected) {
            printf("FAIL: mem[%x]: %d != %d\n", v.idx, got, expected);
            return true;
        }
    }
    return false;
}

static bool itype() {
    VerilatedVcdC *trace = nullptr;
    auto dut = std::make_unique<VCore>();
#ifdef TRACE
    trace = new VerilatedVcdC;
    dut->trace(trace, 99);
    trace->open("riscinator-itype.vcd");
#endif

    uint32_t mem[MEMSIZE];
    memset(mem, 0, sizeof(mem));
    memcpy(mem, itype_bin, itype_bin_len);
    check_t* check = (check_t*) calloc(sizeof(check_t), 1);
    check->regs = {
        {1, 63},
        {2, 3},
        {3, 67},
        {4, 3},
        {5, (uint32_t)-1},
        {7, 1},
        {8, 0x100004},
        {9, 63},
        {10, 3},
        {11, 67},
    };
    check->mem = {
        {0x100000, 3},
        {0x100004, 63},
        {0x100008, 67},
    };
    simulate(*dut.get(), trace, (uint32_t*) mem, MEMSIZE, MEMBASE, 50, false);
    return check_results(*dut.get(), (uint32_t*) itype_bin, MEMBASE, check);
}

static bool jmps() {
    VerilatedVcdC *trace = nullptr;
    auto dut = std::make_unique<VCore>();
#ifdef TRACE
    trace = new VerilatedVcdC;
    dut->trace(trace, 99);
    trace->open("riscinator-jmps.vcd");
#endif

    uint32_t mem[MEMSIZE];
    memset(mem, 0, sizeof(mem));
    memcpy(mem, jmps_bin, jmps_bin_len);
    check_t* check = (check_t*) calloc(sizeof(check_t), 1);
    check->regs = {
        {1, 0},
        {2, 3},
        {3, 67},
        {4, 3},
        {5, (uint32_t)-1},
        {7, 1},
        {8, 0x100004},
        {9, 63},
        {10, 3},
        {11, 67},
        {13, 1048688},
        {17, 1},
        {19, 1},
        {20, 1},
        {21, 1},
        {22, 1},
        {23, 1},
        {25, 1},
        {26, 63},
    };
    check->mem = {
        {0x100000, 3},
        {0x100004, 63},
        {0x100008, 67},
    };
    simulate(*dut.get(), trace, (uint32_t*) mem, MEMSIZE, MEMBASE, 50, false);
    return check_results(*dut.get(), (uint32_t*) jmps_bin, MEMBASE, check);
}

static bool dhrystone() {
    auto dut = std::make_unique<VCore>();
    VerilatedVcdC *trace = nullptr;
#ifdef TRACE
    trace = new VerilatedVcdC;
    dut->trace(trace, 99);
    trace->open("riscinator-dhrystone.vcd");
#endif

    uint32_t mem[MEMSIZE];
    memset(mem, 0, sizeof(mem));
    memcpy(mem, dhrystone_bin, dhrystone_bin_len);

    finished = false;
    printf("%f\n", simulate(*dut.get(), trace, (uint32_t*) mem, MEMSIZE, MEMBASE, 10000000, false));
    return !finished;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc,argv);
    Verilated::debug(0);
#ifdef TRACE
    Verilated::traceEverOn(true);
#endif
    bool failed = false;
    if (argc > 2) {
        fprintf(stderr, "Format: riscinator-main (itype|jmps|dhrystone)?\n");
        return 0;
    }
#ifdef TRACE
    fprintf(stderr, "Tracing enabled!\n");
#endif
    if (argc == 1 || !std::strcmp(argv[1], "itype")) {
        fprintf(stderr, "** start simulating itype **\n");
        failed |= itype();
    }
    if (argc == 1 || !std::strcmp(argv[1], "jmps")) {
        fprintf(stderr, "** start simulating jmps **\n");
        failed |= jmps();
    }
    if (argc == 1 || !std::strcmp(argv[1], "dhrystone")) {
        fprintf(stderr, "** start simulating dhrystone **\n");
        for (int i = 0; i < 10; ++i)
            failed |= dhrystone();
    }

    if (failed) return 1;
    fprintf(stderr, "ALL TESTS PASSED\n");
    return 0;
}
