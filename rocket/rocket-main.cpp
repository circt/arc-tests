#include "elfio/elfio.hpp"
#include "rocket-model.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>

RocketModel::~RocketModel() {}

class ComparingRocketModel : public RocketModel {
public:
  std::vector<std::unique_ptr<RocketModel>> models;
  size_t cycle = 0;
  size_t num_mismatches = 0;

  virtual ~ComparingRocketModel() {
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
    compare_ports();
    vcd_dump(cycle);
    set_clock(true);
    eval();
    set_clock(false);
    eval();
    ++cycle;
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
  enum {
    RESP_OKAY = 0b00,
    RESP_EXOKAY = 0b01,
    RESP_SLVERR = 0b10,
    RESP_DECERR = 0b11
  };

  AxiInputs in;
  AxiOutputs out;

  void update_a();
  void update_b();

  std::function<void(size_t addr, size_t &data)> readFn;
  std::function<void(size_t addr, size_t data, size_t mask)> writeFn;

private:
  unsigned read_beats_left = 0;
  size_t read_id;
  size_t read_addr;
  size_t read_size; // log2
  unsigned write_beats_left = 0;
  size_t write_id;
  size_t write_addr;
  size_t write_size; // log2
  bool write_acked = true;
};

void AxiPort::update_a() {
  // Present read data.
  in.r_valid = false;
  in.r_id = 0;
  in.r_data = 0;
  in.r_resp = RESP_OKAY;
  in.r_last = false;
  if (read_beats_left > 0) {
    in.r_valid = true;
    in.r_id = read_id;
    if (readFn)
      readFn(read_addr, in.r_data);
    else
      in.r_data = 0x1050007310500073; // wfi
    in.r_last = read_beats_left == 1;
  }

  // Present write acknowledge.
  in.b_valid = false;
  in.b_id = 0;
  in.b_resp = RESP_OKAY;
  if (write_beats_left == 0 && !write_acked) {
    in.b_valid = true;
    in.b_id = write_id;
  }

  // Handle write data.
  in.w_ready = write_beats_left > 0;
  if (out.w_valid && in.w_ready) {
    if (writeFn) {
      size_t strb = out.w_strb;
      strb &= ((1 << (1 << write_size)) - 1) << (write_addr % 8);
      writeFn(write_addr, out.w_data, strb);
    }
    assert(out.w_last == (write_beats_left == 1));
    --write_beats_left;
    write_addr = ((write_addr >> write_size) + 1) << write_size;
  }

  in.aw_ready = write_beats_left == 0 && write_acked;
  in.ar_ready = read_beats_left == 0;

  // Accept new reads.
  if (out.ar_valid && in.ar_ready) {
    read_beats_left = out.ar_len + 1;
    read_id = out.ar_id;
    read_addr = out.ar_addr;
    read_size = out.ar_size;
  }

  // Accept new writes.
  if (out.aw_valid && in.aw_ready) {
    write_beats_left = out.aw_len + 1;
    write_id = out.aw_id;
    write_addr = out.aw_addr;
    write_size = out.aw_size;
    write_acked = false;
  }
}

void AxiPort::update_b() {
  if (in.r_valid && out.r_ready) {
    --read_beats_left;
    read_addr = ((read_addr >> read_size) + 1) << read_size;
  }

  if (in.b_valid && out.b_ready) {
    write_acked = true;
  }
}

int main(int argc, char **argv) {
  //===--------------------------------------------------------------------===//
  // Process CLI arguments
  //===--------------------------------------------------------------------===//

  bool optRunAll = true;
  bool optRunArcs = false;
  bool optRunVtor = false;
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

  // Allocate the simulation models.
  ComparingRocketModel model;
  if (optRunAll || optRunVtor)
    model.models.push_back(makeVerilatorModel());
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

  size_t num_bad_cycles = 0;
  for (unsigned i = 0; i < 50000; ++i) {
    mem_port.out = model.get_mem();
    mem_port.update_a();
    model.set_mem(mem_port.in);
    model.eval();
    mem_port.out = model.get_mem();
    mem_port.update_b();
    model.set_mem(mem_port.in);
    model.clock();

    if (model.num_mismatches > 0) {
      if (++num_bad_cycles >= 3) {
        std::cerr << "aborting due to port mismatches\n";
        return 1;
      }
    }
  }

  return 0;
}
