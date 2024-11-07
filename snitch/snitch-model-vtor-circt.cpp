#include "snitch-model.h"
#include "snitch-vtor-circt.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorSnitchModel : public SnitchModel {
  VSnitchCirct model;
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

  void set_inst(const Inst &in) override {
    model.inst_ready_i = in.ready_i;
    model.inst_data_i = in.data_i;
  }

  void get_inst(Inst &out) override {
    out.valid_o = model.inst_valid_o;
    out.addr_o = model.inst_addr_o;
  }

  void set_mem(const MemInputs &in) override {
    model.mem_ready_i = in.mem_ready_i;
    model.mem_rdata_i = in.mem_rdata_i;
  }

  MemOutputs get_mem() override {
    MemOutputs out;
    out.mem_valid_o = model.mem_valid_o;
    out.mem_addr_o = model.mem_addr_o;
    out.mem_write_o = model.mem_write_o;
    out.mem_wdata_o = model.mem_wdata_o;
    out.mem_wstrb_o = model.mem_wstrb_o;
    return out;
  }
};
} // namespace

std::unique_ptr<SnitchModel> makeVerilatorCirctModel() {
  return std::make_unique<VerilatorSnitchModel>();
}
