#include "elfio/elfio.hpp"
#include "snitch-model.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>

#define TOHOST_ADDR 0x60000000
#define FROMHOST_ADDR 0x60000040
#define TOHOST_DATA_ADDR 0x60000080
#define TOHOST_DATA_SIZE 64 // bytes
#define SYS_write 64
#define BOOTROM_BASE 0x10000

double sc_time_stamp() { return 0; }

static bool finished = false;

SnitchModel::~SnitchModel() {}

class ComparingSnitchModel : public SnitchModel {
public:
  std::vector<std::unique_ptr<SnitchModel>> models;
  size_t cycle = 0;
  size_t num_mismatches = 0;

  virtual ~ComparingSnitchModel() {
    std::cerr << "----------------------------------------\n";
    std::cerr << cycle << " cycles total\n";
    for (auto &model : models) {
      auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                         model->duration)
                         .count();
      std::cerr << model->name << ": " << (cycle / seconds) << " Hz\n";
    }
  }

  void vcd_start(const char *outputFile) override {
    for (auto &model : models) {
      std::string extendedFile{outputFile};
      auto pos = extendedFile.rfind('.');
      extendedFile.insert(pos, model->name);
      extendedFile.insert(pos, "-");
      model->vcd_start(extendedFile.data());
    }
  }

  void vcd_dump(size_t cycle) override {
    for (auto &model : models)
      model->vcd_dump(cycle);
  }

  void clock() {
    eval();
    for (unsigned i = 0; i < 2; ++i) {
      compare_ports();
      vcd_dump(cycle);
      ++cycle;
      set_clock(i == 1);
      eval();
    }
  }

  void eval() override {
    for (auto &model : models) {
      auto t_before = std::chrono::high_resolution_clock::now();
      model->eval();
      auto t_after = std::chrono::high_resolution_clock::now();
      model->duration += t_after - t_before;
    }
  }

  void set_clock(bool clock) override {
    for (auto &model : models)
      model->set_clock(clock);
  }

  void set_reset(bool reset) override {
    for (auto &model : models)
      model->set_reset(reset);
  }

  void set_inst(const Inst &in) override {
    for (auto &model : models)
      model->set_inst(in);
  }

  void get_inst(Inst &out) override {
    if (!models.empty())
      models[0]->get_inst(out);
  }

  void set_mem(const MemInputs &in) override {
    for (auto &model : models)
      model->set_mem(in);
  }

  MemOutputs get_mem() override {
    if (models.empty())
      return {};
    return models[0]->get_mem();
  }

  void compare_ports() {
    if (models.size() < 2)
      return;
    auto portsA = models[0]->get_ports();
    for (unsigned modelIdx = 1; modelIdx < models.size(); ++modelIdx) {
      auto portsB = models[modelIdx]->get_ports();
      for (unsigned portIdx = 0; portIdx < portsA.size(); ++portIdx) {
        if (portsA[portIdx] == portsB[portIdx])
          continue;
        ++num_mismatches;
        std::cerr << "cycle " << cycle << ": mismatching " << std::hex
                  << PORT_NAMES[portIdx] << ": " << portsA[portIdx] << " ("
                  << models[0]->name << ") != " << portsB[portIdx] << " ("
                  << models[modelIdx]->name << ")\n"
                  << std::dec;
      }
    }
  }
};

int main(int argc, char **argv) {
  //===--------------------------------------------------------------------===//
  // Process CLI arguments
  //===--------------------------------------------------------------------===//

  bool optRunAll = true;
  bool optRunArcs = false;
  bool optRunVtor = false;
  bool optRunVtorCirct = false;
  char *optVcdOutputFile = nullptr;

  char **argOut = argv + 1;
  for (char **arg = argv + 1, **argEnd = argv + argc; arg != argEnd; ++arg) {
    if (strcmp(*arg, "--arcs") == 0) {
      optRunAll = false;
      optRunArcs = true;
      continue;
    }
    if (strcmp(*arg, "--vtor") == 0) {
      optRunAll = false;
      optRunVtor = true;
      continue;
    }
    if (strcmp(*arg, "--vtor-circt") == 0) {
      optRunAll = false;
      optRunVtorCirct = true;
      continue;
    }
    if (strcmp(*arg, "--trace") == 0) {
      ++arg;
      if (arg == argEnd) {
        std::cerr << "missing trace output file name after `--trace`\n";
        return 1;
      }
      optVcdOutputFile = *arg;
      continue;
    }
    *argOut++ = *arg;
  }
  argc = argOut - argv;

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " [options] <binary>\n";
    std::cerr << "options:\n";
    std::cerr << "  --arcs         run arcilator simulation\n";
    std::cerr << "  --vtor         run verilator simulation\n";
    std::cerr << "  --vtor-circt   run verilator round-trip simulation\n";
    std::cerr << "  --trace <VCD>  write trace to <VCD> file\n";
    return 1;
  }

  //===--------------------------------------------------------------------===//
  // Read ELF into memory
  //===--------------------------------------------------------------------===//

  std::map<uint32_t, uint64_t> memory;
  {
    ELFIO::elfio elf;
    if (!elf.load(argv[1])) {
      std::cerr << "unable to open file " << argv[1] << std::endl;
      return 1;
    }
    std::cerr << std::hex;
    for (const auto &segment : elf.segments) {
      if (segment->get_type() != ELFIO::PT_LOAD ||
          segment->get_memory_size() == 0)
        continue;
      std::cerr << "loading segment at " << segment->get_physical_address()
                << " (virtual address " << segment->get_virtual_address()
                << ")\n";
      for (unsigned i = 0; i < segment->get_memory_size(); ++i) {
        uint32_t addr = segment->get_physical_address() + i;
        uint8_t data = 0;
        if (i < segment->get_file_size())
          data = segment->get_data()[i];
        constexpr unsigned bpw = sizeof(memory[0]);
        auto &slot = memory[addr / bpw * bpw];
        slot &= ~((uint64_t)0xFF << ((addr % bpw) * 8));
        slot |= (uint64_t)data << ((addr % bpw) * 8);
      }
    }
    std::cerr << "entry " << elf.get_entry() << "\n";
    std::cerr << std::dec;
    std::cerr << "loaded " << memory.size() * sizeof(memory[0])
              << " program bytes\n";
  }

  // Place bootrom.
  memory[BOOTROM_BASE + 0] = 0;
  memory[BOOTROM_BASE + 0] |= 0x80000537UL;       // lui a0, 0x80000
  memory[BOOTROM_BASE + 0] |= 0x00050067UL << 32; // jr a0  # 0x80000000

  // Allocate the simulation models.
  ComparingSnitchModel model;
  if (optRunAll || optRunVtor)
    model.models.push_back(makeVerilatorModel());
  if (optRunAll || optRunVtorCirct)
    model.models.push_back(makeVerilatorCirctModel());
  if (optRunAll || optRunArcs)
    model.models.push_back(makeArcilatorModel());
  if (optVcdOutputFile)
    model.vcd_start(optVcdOutputFile);

  //===--------------------------------------------------------------------===//
  // Model initialization and reset
  //===--------------------------------------------------------------------===//

  for (unsigned i = 0; i < 1000; ++i) {
    model.set_reset(i < 100);
    model.clock();
  }

  //===--------------------------------------------------------------------===//
  // Simulation loop
  //===--------------------------------------------------------------------===//

  // AxiPort mem_port;
  // mem_port.readFn = [&](size_t addr, size_t &data) {
  //   auto it = memory.find(addr / 8 * 8);
  //   if (it != memory.end())
  //     data = it->second;
  //   else
  //     data = 0x1050007310500073;
  // };
  // mem_port.writeFn = [&](size_t addr, size_t data, size_t mask) {
  //   assert(mask == 0xFF && "only full 64 bit write supported");
  //   memory[addr / 8 * 8] = data;
  // };

  // AxiPort mmio_port;
  // mmio_port.writeFn = [&](size_t addr, size_t data, size_t mask) {
  //   assert(mask == 0xFF && "only full 64 bit write supported");
  //   memory[addr / 8 * 8] = data;

  //   // For a zero return code from the main function, 1 is written to tohost.
  //   if (addr == TOHOST_ADDR) {
  //     if (data == 1) {
  //       finished = true;
  //       std::cout << "Benchmark run successful!\n";
  //       return;
  //     }

  //     if (data == SYS_write) {
  //       for (int i = 0; i < TOHOST_DATA_SIZE; i += 8) {
  //         uint64_t data = memory[TOHOST_DATA_ADDR + i];
  //         unsigned char c[8];
  //         *(uint64_t *)c = data;
  //         for (int k = 0; k < 8; ++k) {
  //           std::cout << c[k];
  //           if ((unsigned char)c[k] == 0)
  //             return;
  //         }
  //       }
  //     }
  //   }
  // };
  // mmio_port.readFn = [&](size_t addr, size_t &data) {
  //   // Core loops on condition fromhost=0, thus set it to something non-zero.
  //   if (addr == FROMHOST_ADDR)
  //     data = -1;
  // };

  size_t num_bad_cycles = 0;
  for (unsigned i = 0; i < 1000; ++i) {
    // Service the instruction interface.
    Inst inst = {};
    model.get_inst(inst);
    inst.ready_i = true;
    if (inst.valid_o) {
      uint64_t data = -1;
      if (auto it = memory.find(inst.addr_o / 8 * 8); it != memory.end()) {
        data = it->second;
      } else {
        std::cerr << "cycle " << model.cycle
                  << ": instruction fetch from unmapped address " << std::hex
                  << inst.addr_o << std::dec << "\n";
      }
      inst.data_i = data >> ((inst.addr_o / 4) * 32);
      std::cerr << "cycle " << model.cycle << ": fetched [0x" << std::hex
                << inst.addr_o << "] = 0x" << inst.data_i << std::dec << "\n";
    }
    model.set_inst(inst);

    // Service the memory interface.
    // auto out = model.get_mem();
    // MemInputs in = {};
    // in.mem_ready_i = false;
    // if (out.mem_valid_o) {
    //   uint64_t &data = memory[out.mem_addr_o / 8 * 8];
    //   if (out.mem_write_o) {
    //     uint64_t mask = 0;
    //     for (unsigned i = 0; i < 8; ++i)
    //       if ((out.mem_wstrb_o >> i) & 1)
    //         mask |= 0xFFull << (i * 8);
    //     data = (data & ~mask) | (out.mem_wdata_o & mask);
    //   }
    //   in.mem_rdata_i = data;

    //   const char *word = out.mem_write_o ? "writing" : "reading";
    //   std::cerr << "cycle " << model.cycle << ": " << word << " [0x" <<
    //   std::hex
    //             << (out.mem_addr_o / 8 * 8) << "] m"
    //             << (uint16_t)out.mem_wstrb_o << " = 0x" << data << std::dec
    //             << "\n";
    // }
    // model.set_mem(in);

    // Toggle the clock.
    model.clock();

    if (finished)
      return 0;

    if (model.num_mismatches > 0) {
      if (++num_bad_cycles >= 3) {
        std::cerr << "aborting due to port mismatches\n";
        return 1;
      }
    }
  }

  return 0;
}
