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

  void set_mem(AxiInputs &in) override {
    for (auto &model : models)
      model->set_mem(in);
  }

  AxiOutputs get_mem() override {
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

struct AxiPort {
  AxiInputs in;
  AxiOutputs out;

  void update();

  std::function<void(size_t addr, size_t &data)> readFn;
  std::function<void(size_t addr, size_t data, size_t mask)> writeFn;

private:
  unsigned data_resp_pending = 0;
};

void AxiPort::update() {
  if (in.data_pvalid_i && out.data_pready_o)
    --data_resp_pending;
  if (out.data_qvalid_o && in.data_qready_i)
    ++data_resp_pending;

  in.data_qready_i = true;
  if (readFn)
    readFn(out.data_qaddr_o, in.data_pdata_i);
  else
    in.data_pdata_i = 0x1050007310500073; // wfi

  in.data_perror_i = false;
  in.data_pvalid_i = data_resp_pending > 0;

  if (readFn)
    readFn(out.inst_addr_o, in.inst_data_i);
  else
    in.inst_data_i = 0x1050007310500073; // wfi

  in.inst_ready_i = true;

  // // Handle write data.
  // in.w_ready = write_beats_left > 0;
  // if (out.w_valid && in.w_ready) {
  //   if (writeFn) {
  //     size_t strb = out.w_strb;
  //     strb &= ((1 << (1 << write_size)) - 1) << (write_addr % 8);
  //     writeFn(write_addr, out.w_data, strb);
  //   }
  //   assert(out.w_last == (write_beats_left == 1));
  //   --write_beats_left;
  //   write_addr = ((write_addr >> write_size) + 1) << write_size;
  // }

  // in.aw_ready = write_beats_left == 0 && write_acked;
  // in.ar_ready = read_beats_left == 0;

  // // Accept new reads.
  // if (out.ar_valid && in.ar_ready) {
  //   read_beats_left = out.ar_len + 1;
  //   read_id = out.ar_id;
  //   read_addr = out.ar_addr;
  //   read_size = out.ar_size;
  // }

  // // Accept new writes.
  // if (out.aw_valid && in.aw_ready) {
  //   write_beats_left = out.aw_len + 1;
  //   write_id = out.aw_id;
  //   write_addr = out.aw_addr;
  //   write_size = out.aw_size;
  //   write_acked = false;
  // }
}

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
    std::cerr << "  --trace <VCD>  write trace to <VCD> file\n";
    return 1;
  }

  //===--------------------------------------------------------------------===//
  // Read ELF into memory
  //===--------------------------------------------------------------------===//

  std::map<uint64_t, uint64_t> memory;
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
        uint64_t addr = segment->get_physical_address() + i;
        uint8_t data = 0;
        if (i < segment->get_file_size())
          data = segment->get_data()[i];
        auto &slot = memory[addr / 8 * 8];
        slot &= ~((uint64_t)0xFF << ((addr % 8) * 8));
        slot |= (uint64_t)data << ((addr % 8) * 8);
      }
    }
    std::cerr << "entry " << elf.get_entry() << "\n";
    std::cerr << std::dec;
    std::cerr << "loaded " << memory.size() * 8 << " program bytes\n";
  }

  // Place bootrom
	memory[BOOTROM_BASE+0] = 0x3e800713; // li      a4,1000
	memory[BOOTROM_BASE+1] = 0x00100793; // li      a5,1
	memory[BOOTROM_BASE+2] = 0x00000613; // li      a2,0
	memory[BOOTROM_BASE+3] = 0xcafe15b7; // lui     a1,0xcafe1
	memory[BOOTROM_BASE+4] = 0x00f606b3; // add     a3,a2,a5
	memory[BOOTROM_BASE+5] = 0x00d5a023; // sw      a3,0(a1) # cafe1000 <__global_pointer$+0xcafcf7cc>
	memory[BOOTROM_BASE+6] = 0xfff70713; // addi    a4,a4,-1
	memory[BOOTROM_BASE+7] = 0x00078613; // mv      a2,a5
	memory[BOOTROM_BASE+8] = 0x00068793; // mv      a5,a3
	memory[BOOTROM_BASE+9] = 0xfe0716e3; // bnez    a4,10010 <_start+0x10>
	memory[BOOTROM_BASE+10] = 0x10500073; // wfi
	memory[BOOTROM_BASE+11] = 0xffdff06f; // j       10028 <_start+0x28>
	memory[BOOTROM_BASE+12] = 0x00008067; // ret

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

  AxiPort mem_port;
  mem_port.readFn = [&](size_t addr, size_t &data) {
    auto it = memory.find(addr / 8 * 8);
    if (it != memory.end())
      data = it->second;
    else
      data = 0x1050007310500073;
  };
  mem_port.writeFn = [&](size_t addr, size_t data, size_t mask) {
    assert(mask == 0xFF && "only full 64 bit write supported");
    memory[addr / 8 * 8] = data;
  };


  AxiPort mmio_port;
  mmio_port.writeFn = [&](size_t addr, size_t data, size_t mask) {
    assert(mask == 0xFF && "only full 64 bit write supported");
    memory[addr / 8 * 8] = data;

    // For a zero return code from the main function, 1 is written to tohost.
    if (addr == TOHOST_ADDR) {
      if (data == 1) {
        finished = true;
        std::cout << "Benchmark run successful!\n";
        return;
      }

      if (data == SYS_write) {
        for (int i = 0; i < TOHOST_DATA_SIZE; i += 8) {
          uint64_t data = memory[TOHOST_DATA_ADDR + i];
          unsigned char c[8];
          *(uint64_t*) c = data;
          for (int k = 0; k < 8; ++k) {
            std::cout << c[k];
            if ((unsigned char) c[k] == 0)
              return;
          }
        }
      }
    }
  };
  mmio_port.readFn = [&](size_t addr, size_t &data) {
    // Core loops on condition fromhost=0, thus set it to something non-zero.
    if (addr == FROMHOST_ADDR)
      data = -1;
  };

  size_t num_bad_cycles = 0;
  for (unsigned i = 0; i < 1000000; ++i) {
    mem_port.out = model.get_mem();
    mem_port.update();
    model.set_mem(mem_port.in);

    // model.eval();
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
