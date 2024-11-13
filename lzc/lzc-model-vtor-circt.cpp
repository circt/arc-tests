#include "lzc-model.h"
#include "lzc-vtor-circt.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorLzcModel : public LzcModel {
  VLzcCirct model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorLzcModel() { name = "vtor-circt"; }
  ~VerilatorLzcModel() {
    if (model_vcd)
      model_vcd->close();
  }

  void vcd_start(const char *outputFile) override {
    Verilated::traceEverOn(true);
    model_vcd = std::make_unique<VerilatedVcdC>();
#ifdef TRACE
    model.trace(model_vcd.get(), 10000);
#endif
    model_vcd->open(outputFile);
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd)
      model_vcd->dump(static_cast<uint64_t>(cycle));
  }

  void eval() override { model.eval(); }

  Ports get_ports() override {
    return {
#define PORT(name) model.name,
#include "ports.def"
    };
  }

  uint32_t getCount() override {
    return model.cnt_o;
  }
 
  bool getEmpty() override {
    return model.empty_o;
  }

  void setIn(uint32_t in) override {
    model.in_i = in;
  }
};
} // namespace

std::unique_ptr<LzcModel> makeVerilatorCirctModel() {
  return std::make_unique<VerilatorLzcModel>();
}
