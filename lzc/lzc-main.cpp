#include "lzc-model.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>

double sc_time_stamp() { return 0; }

LzcModel::~LzcModel() {}

class ComparingLzcModel : public LzcModel {
public:
  std::vector<std::unique_ptr<LzcModel>> models;
  size_t cycle = 0;
  size_t num_mismatches = 0;

  virtual ~ComparingLzcModel() {
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
    compare_ports();
    vcd_dump(cycle);
    ++cycle;
  }

  uint32_t getCount() override {
    return models[0]->getCount();
  }
 
  bool getEmpty() override {
    return models[0]->getEmpty();
  }

  void setIn(uint32_t in) override {
    for (auto &model : models)
      model->setIn(in);
  }

  void eval() override {
    for (auto &model : models) {
      auto t_before = std::chrono::high_resolution_clock::now();
      model->eval();
      auto t_after = std::chrono::high_resolution_clock::now();
      model->duration += t_after - t_before;
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

  // Allocate the simulation models.
  ComparingLzcModel model;
  if (optRunAll || optRunVtor)
    model.models.push_back(makeVerilatorModel());
  if (optRunAll || optRunVtorCirct)
    model.models.push_back(makeVerilatorCirctModel());
  if (optRunAll || optRunArcs)
    model.models.push_back(makeArcilatorModel());
  if (optVcdOutputFile)
    model.vcd_start(optVcdOutputFile);

  //===--------------------------------------------------------------------===//
  // Simulation loop
  //===--------------------------------------------------------------------===//

  size_t num_bad_cycles = 0;
  for (unsigned i = 0; i < 1000000; ++i) {
    model.setIn(i);
    model.clock();

    // assert (model.getCount() == std::__countl_zero(i));
    // assert (model.getEmpty() == (i == 0));

    if (model.num_mismatches > 0) {
      if (++num_bad_cycles >= 3) {
        std::cerr << "aborting due to port mismatches\n";
        return 1;
      }
    }
  }

  return 0;
}
