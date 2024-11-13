#include "graycode-model.h"
#include "graycode-vtor.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorGraycodeModel : public GraycodeModel {
  VGraycode model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorGraycodeModel() { name = "vtor"; }
  ~VerilatorGraycodeModel() {
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

  uint32_t getG() override {
    return model.G;
  }
 
  uint32_t getZ() override {
    return model.Z;
  }

  void setA(uint32_t A) override {
    model.A = A;
  }
};
} // namespace

std::unique_ptr<GraycodeModel> makeVerilatorModel() {
  return std::make_unique<VerilatorGraycodeModel>();
}
