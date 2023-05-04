#include "riscinator.h"
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <stdlib.h>
#include <cassert>
#include <cstring>
#include <vector>

#include "itype_mem.h"
#include "jmps_mem.h"

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

static bool failed = false;

static void simulate(Core &model, ValueChangeDump<CoreLayout> vcd, uint32_t* mem, size_t len, size_t mem_base, check_t* check) {
    auto &core = model.view;
    core.reset = 1;
    model.passthrough();
    vcd.writeTimestep(1);
    model.clock();
    model.passthrough();
    vcd.writeTimestep(1);
    core.reset = 0;

    uint32_t next_imem_rdata = 0;
    uint8_t next_imem_rvalid = 0;
    uint32_t next_dmem_rdata = 0;
    uint8_t next_dmem_rvalid = 0;

    unsigned ncyc = 50;
    for (unsigned i = 0; i < ncyc; i++) {
        model.passthrough();
        vcd.writeTimestep(1);

        core.io_imem_rvalid = next_imem_rvalid;
        core.io_imem_rdata = next_imem_rdata;
        core.io_dmem_rvalid = next_dmem_rvalid;
        core.io_dmem_rdata = next_dmem_rdata;

        next_imem_rvalid = core.io_imem_req;
        next_dmem_rvalid = core.io_dmem_req;
        core.io_dmem_gnt = core.io_dmem_req;
        model.passthrough();

        if (core.io_imem_req) {
            assert(addr2idx(core.io_imem_addr, mem_base) < len);
            next_imem_rdata = mem[addr2idx(core.io_imem_addr, mem_base)];
        }

        if (core.io_dmem_req && core.io_dmem_we) {
            uint32_t write = core.io_dmem_wdata;
            uint32_t mask = be2mask(core.io_dmem_be);
            assert(addr2idx(core.io_dmem_addr, mem_base) < len);
            mem[addr2idx(core.io_dmem_addr, mem_base)] = write & mask;
        } else if (core.io_dmem_req) {
            assert(addr2idx(core.io_dmem_addr, mem_base) < len);
            next_dmem_rdata = mem[addr2idx(core.io_dmem_addr, mem_base)];
        }

        model.clock();
    }

    for (auto v : check->regs) {
        auto got = core.internal.rf.regs_ext.words[v.idx].data;
        auto expected = v.value;
        if (got != expected) {
            printf("FAIL: regs[%d]: %d != %d\n", v.idx, got, expected);
            failed = true;
        }
    }
    for (auto v : check->mem) {
        auto got = mem[addr2idx(v.idx, mem_base)];
        auto expected = v.value;
        if (got != expected) {
            printf("FAIL: mem[%x]: %d != %d\n", v.idx, got, expected);
            failed = true;
        }
    }
}

#define MEMSIZE 4096
#define MEMBASE 0x100000
static uint32_t mem[MEMSIZE];

static void itype() {
    Core core;
    std::ofstream os("itype.vcd");
    auto vcd = core.vcd(os);

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
    simulate(core, vcd, (uint32_t*) itype_bin, MEMSIZE, MEMBASE, check);
}

static void jmps() {
    Core core;
    std::ofstream os("jmps.vcd");
    auto vcd = core.vcd(os);

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
    simulate(core, vcd, (uint32_t*) jmps_bin, MEMSIZE, MEMBASE, check);
}

int main(int argc, char **argv) {
    itype();
    jmps();

    if (failed) return 1;
    printf("ALL TESTS PASSED\n");
    return 0;
}
