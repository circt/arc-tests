#include "lzc-arc.h"
#include "lzc-model.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorLzcModel : public LzcModel {
  lzc model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<lzcLayout>> model_vcd;

public:
  ArcilatorLzcModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    model_vcd.reset(
        new ValueChangeDump<lzcLayout>(model.vcd(vcd_stream)));
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      model_vcd->time = cycle;
      model_vcd->writeTimestep(0);
    }
  }

  void eval() override { lzc_eval(&model.storage[0]); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  uint32_t getCount() override {
    return model.view.cnt_o;
  }
 
  bool getEmpty() override {
    return model.view.empty_o;
  }

  void setIn(uint32_t in) override {
    model.view.in_i = in;
  }
};
} // namespace

std::unique_ptr<LzcModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorLzcModel>();
}
