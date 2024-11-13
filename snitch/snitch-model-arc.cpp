#include "snitch-arc.h"
#include "snitch-model.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorSnitchModel : public SnitchModel {
  snitch_th model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<snitch_thLayout>> model_vcd;

public:
  ArcilatorSnitchModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    model_vcd.reset(
        new ValueChangeDump<snitch_thLayout>(model.vcd(vcd_stream)));
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      model_vcd->time = cycle;
      model_vcd->writeTimestep(0);
    }
  }

  void eval() override { snitch_th_eval(&model.storage[0]); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  void set_reset(bool reset) override { model.view.rst_i = reset; }

  void set_clock(bool clock) override { model.view.clk_i = clock; }

  void set_mem(const MemInputs &in) override {
    model.view.mem_ready_i = in.mem_ready_i;
    model.view.mem_rdata_i = in.mem_rdata_i;
  }

  MemOutputs get_mem() override {
    MemOutputs out;
    out.mem_valid_o = model.view.mem_valid_o;
    out.mem_addr_o = model.view.mem_addr_o;
    out.mem_write_o = model.view.mem_write_o;
    out.mem_wdata_o = model.view.mem_wdata_o;
    out.mem_wstrb_o = model.view.mem_wstrb_o;
    return out;
  }
};
} // namespace

std::unique_ptr<SnitchModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorSnitchModel>();
}
