#include "lfsr_16bit-arc.h"
#include "lfsr_16bit-model.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorLfsr_16bitModel : public Lfsr_16bitModel {
  lfsr_16bit model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<lfsr_16bitLayout>> model_vcd;

public:
  ArcilatorLfsr_16bitModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    model_vcd.reset(
        new ValueChangeDump<lfsr_16bitLayout>(model.vcd(vcd_stream)));
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      model_vcd->time = cycle;
      model_vcd->writeTimestep(0);
    }
  }

  void eval(bool advance_clock = false) override { lfsr_16bit_eval(&model.storage[0]); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  uint32_t getOut() override {
    return model.view.out_o;
  }

  void setClock(bool clock) override {
    model.view.clk_i = clock;
  }

  void setReset(bool reset) override {
    model.view.rst_ni = reset;
  }

  void setEnable(bool enable) override {
    model.view.en_i = enable;
  }
};
} // namespace

std::unique_ptr<Lfsr_16bitModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorLfsr_16bitModel>();
}
