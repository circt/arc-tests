#include "lfsr_16bit-model.h"
#include "lfsr_16bit-vtor.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorLfsr_16bitModel : public Lfsr_16bitModel {
  VLfsr_16bit model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorLfsr_16bitModel() { name = "vtor"; }
  ~VerilatorLfsr_16bitModel() {
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

  void eval(bool advance_clock = false) override { model.eval(); }

  Ports get_ports() override {
    return {
#define PORT(name) model.name,
#include "ports.def"
    };
  }

  uint32_t getOut() override {
    return model.out_o;
  }

  void setClock(bool clock) override {
    model.clk_i = clock;
  }

  void setReset(bool reset) override {
    model.rst_ni = reset;
  }

  void setEnable(bool enable) override {
    model.en_i = enable;
  }
};
} // namespace

std::unique_ptr<Lfsr_16bitModel> makeVerilatorModel() {
  return std::make_unique<VerilatorLfsr_16bitModel>();
}
