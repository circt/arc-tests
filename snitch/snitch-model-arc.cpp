#include "snitch-arc.h"
#include "snitch-model.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorSnitchModel : public SnitchModel {
  snitch model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<snitchLayout>> model_vcd;

public:
  ArcilatorSnitchModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    model_vcd.reset(
        new ValueChangeDump<snitchLayout>(model.vcd(vcd_stream)));
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      model_vcd->time = cycle;
      model_vcd->writeTimestep(0);
    }
  }

  void eval() override { snitch_eval(&model.storage[0]); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  void set_reset(bool reset) override { model.view.rst_i = reset; }

  void set_clock(bool clock) override { model.view.clk_i = clock; }

  void set_mem(AxiInputs &in) override {
    model.view.data_qready_i = in.data_qready_i;
    model.view.data_perror_i = in.data_perror_i;
    model.view.data_pvalid_i = in.data_pvalid_i;
    model.view.data_pdata_i = in.data_pdata_i;

    model.view.inst_data_i = in.inst_data_i;
    model.view.inst_ready_i = in.inst_ready_i;
  }

  AxiOutputs get_mem() override {
    AxiOutputs out;
    out.data_qaddr_o = model.view.data_qaddr_o;
    out.data_qwrite_o = model.view.data_qwrite_o;
    out.data_qamo_o = model.view.data_qamo_o;
    out.data_qdata_o = model.view.data_qdata_o;
    out.data_qstrb_o = model.view.data_qstrb_o;
    out.data_qvalid_o = model.view.data_qvalid_o;
    out.data_pready_o = model.view.data_pready_o;

    out.inst_addr_o = model.view.inst_addr_o;
    out.inst_valid_o = model.view.inst_valid_o;
    return out;
  }
};
} // namespace

std::unique_ptr<SnitchModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorSnitchModel>();
}
