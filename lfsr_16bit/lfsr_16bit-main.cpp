#include "lfsr_16bit-model.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>

double sc_time_stamp() { return 0; }

Lfsr_16bitModel::~Lfsr_16bitModel() {}

class ComparingLfsr_16bitModel : public Lfsr_16bitModel {
public:
  std::vector<std::unique_ptr<Lfsr_16bitModel>> models;
  size_t cycle = 0;
  size_t num_mismatches = 0;

  virtual ~ComparingLfsr_16bitModel() {
    std::cerr << "----------------------------------------\n";
    std::cerr << cycle << " cycles total\n";
    for (auto &model : models) {
      auto seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                         model->duration)
                         .count();
      std::cerr << model->name << ": " << (cycle / seconds) << " Hz\n";
      seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                         model->clock_time)
                         .count();
      std::cerr << model->name << ": time in clock eval " << seconds << " sec ("
                << (cycle / seconds) << " Hz)\n";
      seconds = std::chrono::duration_cast<std::chrono::duration<double>>(
                         model->passthrough_time)
                         .count();
      std::cerr << model->name << ": time in passthrough eval " << seconds <<
                   " sec (" << (cycle / seconds) << " Hz)\n";
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
    setClock(0);
    eval();
    setClock(1);
    eval(true);
    compare_ports();
    vcd_dump(cycle);
    ++cycle;
  }

  uint32_t getOut() override {
    return models[0]->getOut();
  }

  void setClock(bool clock) override {
    for (auto &model : models)
      model->setClock(clock);
  }

  void setReset(bool reset) override {
    for (auto &model : models)
      model->setReset(reset);
  }

  void setEnable(bool enable) override {
    for (auto &model : models)
      model->setEnable(enable);
  }

  void eval(bool advance_clock = false) override {
    for (auto &model : models) {
      auto t_before = std::chrono::high_resolution_clock::now();
      model->eval(advance_clock);
      auto t_after = std::chrono::high_resolution_clock::now();
      model->duration += t_after - t_before;
      if (advance_clock)
        model->clock_time += t_after - t_before;
      else
        model->passthrough_time += t_after - t_before;
    }
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
#ifdef RUN_ARC
    if (strcmp(*arg, "--arcs") == 0) {
      optRunAll = false;
      optRunArcs = true;
      continue;
    }
#endif
#ifdef RUN_VTOR
    if (strcmp(*arg, "--vtor") == 0) {
      optRunAll = false;
      optRunVtor = true;
      continue;
    }
#endif
#ifdef RUN_VTOR_CIRCT
    if (strcmp(*arg, "--vtor-circt") == 0) {
      optRunAll = false;
      optRunVtorCirct = true;
      continue;
    }
#endif
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
#ifdef RUN_ARC
    std::cerr << "  --arcs         run arcilator simulation\n";
#endif
#ifdef RUN_VTOR
    std::cerr << "  --vtor         run verilator simulation\n";
#endif
#ifdef RUN_VTOR_CIRCT
    std::cerr << "  --vtor-circt   run verilator round-trip simulation\n";
#endif
    std::cerr << "  --trace <VCD>  write trace to <VCD> file\n";
    return 1;
  }

  // Allocate the simulation models.
  ComparingLfsr_16bitModel model;
#ifdef RUN_VTOR
  if (optRunAll || optRunVtor)
    model.models.push_back(makeVerilatorModel());
#endif
#ifdef RUN_VTOR_CIRCT
  if (optRunAll || optRunVtorCirct)
    model.models.push_back(makeVerilatorCirctModel());
#endif
#ifdef RUN_ARC
  if (optRunAll || optRunArcs)
    model.models.push_back(makeArcilatorModel());
#endif
  if (optVcdOutputFile)
    model.vcd_start(optVcdOutputFile);

  model.setEnable(1);
  model.setReset(0);
  model.clock();
  model.setEnable(1);
  model.setReset(1);
  model.clock();

  //===--------------------------------------------------------------------===//
  // Simulation loop
  //===--------------------------------------------------------------------===//

  size_t num_bad_cycles = 0;
  for (unsigned i = 0; i < 10'000'000; ++i) {
    model.setEnable(1);
    model.setReset(1);
    model.clock();

    // assert (model.getZ() == i);

    if (model.num_mismatches > 0) {
      if (++num_bad_cycles >= 3) {
        std::cerr << "aborting due to port mismatches\n";
        return 1;
      }
    }
  }

  return 0;
}
