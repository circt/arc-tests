#include "snitch-arc.h"
#include "snitch-model.h"
#include <fstream>
#include <optional>

namespace {
class ArcilatorSnitchModel : public SnitchModel {
  snitch_th model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<snitch_thLayout>> model_vcd;
  bool is_first_vcd_dump = true;

public:
  ArcilatorSnitchModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    // model_vcd.reset(
    //     new ValueChangeDump<snitch_thLayout>(model.vcd(vcd_stream)));
    model_vcd.reset(
        new ValueChangeDump<snitch_thLayout>(vcd_stream, &model.storage[0]));
    model_vcd->writeHeader();
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      vcd_stream << "#" << cycle << "\n";
      model_vcd->writeValues(is_first_vcd_dump);
      is_first_vcd_dump = false;
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

  void set_inst(const Inst &in) override {
    model.view.inst_ready_i = in.ready_i;
    model.view.inst_data_i = in.data_i;
  }

  void get_inst(Inst &out) override {
    out.valid_o = model.view.inst_valid_o;
    out.addr_o = model.view.inst_addr_o;
  }

  void set_data(const Data &in) override {
    model.view.mem_ready_i = in.ready_i;
    model.view.mem_rdata_i = in.rdata_i;
  }

  void get_data(Data &out) override {
    out.valid_o = model.view.mem_valid_o;
    out.addr_o = model.view.mem_addr_o;
    out.write_o = model.view.mem_write_o;
    out.wdata_o = model.view.mem_wdata_o;
    out.wstrb_o = model.view.mem_wstrb_o;
  }
};
} // namespace

std::unique_ptr<SnitchModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorSnitchModel>();
}
