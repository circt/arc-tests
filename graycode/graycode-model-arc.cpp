#include "graycode-arc.h"
#include "graycode-model.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorGraycodeModel : public GraycodeModel {
  graycode model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<graycodeLayout>> model_vcd;

public:
  ArcilatorGraycodeModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    model_vcd.reset(
        new ValueChangeDump<graycodeLayout>(model.vcd(vcd_stream)));
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      model_vcd->time = cycle;
      model_vcd->writeTimestep(0);
    }
  }

  void eval(bool advance_clock = false) override { graycode_eval(&model.storage[0]); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  uint32_t getG() override {
    return model.view.G;
  }
 
  uint32_t getZ() override {
    return model.view.Z;
  }

  void setA(uint32_t A) override {
    model.view.A = A;
  }
};
} // namespace

std::unique_ptr<GraycodeModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorGraycodeModel>();
}
