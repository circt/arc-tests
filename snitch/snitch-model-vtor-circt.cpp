#include "snitch-model.h"
#include "snitch-vtor-circt.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorSnitchModel : public SnitchModel {
  Vsnitch__02dcirct model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorSnitchModel() { name = "vtor-circt"; }
  ~VerilatorSnitchModel() {
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

  void set_reset(bool reset) override { model.rst_i = reset; }

  void set_clock(bool clock) override { model.clk_i = clock; }

  void set_mem(AxiInputs &in) override {
    model.data_qready_i = in.data_qready_i;
    model.data_perror_i = in.data_perror_i;
    model.data_pvalid_i = in.data_pvalid_i;
    model.data_pdata_i = in.data_pdata_i;

    model.inst_data_i = in.inst_data_i;
    model.inst_ready_i = in.inst_ready_i;
  }

  AxiOutputs get_mem() override {
    AxiOutputs out;
    out.data_qaddr_o = model.data_qaddr_o;
    out.data_qwrite_o = model.data_qwrite_o;
    out.data_qamo_o = model.data_qamo_o;
    out.data_qdata_o = model.data_qdata_o;
    out.data_qstrb_o = model.data_qstrb_o;
    out.data_qvalid_o = model.data_qvalid_o;
    out.data_pready_o = model.data_pready_o;

    out.inst_addr_o = model.inst_addr_o;
    out.inst_valid_o = model.inst_valid_o;
    return out;
  }
};
} // namespace

std::unique_ptr<SnitchModel> makeVerilatorCirctModel() {
  return std::make_unique<VerilatorSnitchModel>();
}
