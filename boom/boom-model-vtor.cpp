#include "boom-model.h"
#include "boom-vtor.h"
#include <iostream>
#include <verilated_vcd_c.h>

namespace {
class VerilatorBoomModel : public BoomModel {
  Vboom model;
  std::unique_ptr<VerilatedVcdC> model_vcd;

public:
  VerilatorBoomModel() { name = "vtor"; }
  ~VerilatorBoomModel() {
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

  void vcd_dump(size_t t) override {
    if (model_vcd)
      model_vcd->dump(static_cast<uint64_t>(t));
  }

  void eval() override { model.eval(); }

  Ports get_ports() override {
    return {
#define PORT(name) model.name,
#include "ports.def"
    };
  }

  void set_reset(bool reset) override {
    // clang-format off
    model.io_aggregator_5_reset = reset;
    model.io_aggregator_4_reset = reset;
    model.io_aggregator_3_reset = reset;
    model.io_aggregator_2_reset = reset;
    model.io_aggregator_1_reset = reset;
    model.io_aggregator_0_reset = reset;
    // clang-format on
  }

  void set_clock(bool clock) {
    // clang-format off
    model.io_aggregator_5_clock = clock;
    model.io_aggregator_4_clock = clock;
    model.io_aggregator_3_clock = clock;
    model.io_aggregator_2_clock = clock;
    model.io_aggregator_1_clock = clock;
    model.io_aggregator_0_clock = clock;
    // clang-format on
  }

  void set_mem(AxiInputs &in) override {
    model.mem_axi4_0_aw_ready = in.aw_ready;
    model.mem_axi4_0_w_ready = in.w_ready;
    model.mem_axi4_0_b_valid = in.b_valid;
    model.mem_axi4_0_b_bits_id = in.b_id;
    model.mem_axi4_0_b_bits_resp = in.b_resp;
    model.mem_axi4_0_ar_ready = in.ar_ready;
    model.mem_axi4_0_r_valid = in.r_valid;
    model.mem_axi4_0_r_bits_id = in.r_id;
    model.mem_axi4_0_r_bits_data = in.r_data;
    model.mem_axi4_0_r_bits_resp = in.r_resp;
    model.mem_axi4_0_r_bits_last = in.r_last;
  }

  AxiOutputs get_mem() override {
    AxiOutputs out;
    out.aw_valid = model.mem_axi4_0_aw_valid;
    out.aw_id = model.mem_axi4_0_aw_bits_id;
    out.aw_addr = model.mem_axi4_0_aw_bits_addr;
    out.aw_len = model.mem_axi4_0_aw_bits_len;
    out.aw_size = model.mem_axi4_0_aw_bits_size;
    out.w_valid = model.mem_axi4_0_w_valid;
    out.w_data = model.mem_axi4_0_w_bits_data;
    out.w_strb = model.mem_axi4_0_w_bits_strb;
    out.w_last = model.mem_axi4_0_w_bits_last;
    out.b_ready = model.mem_axi4_0_b_ready;
    out.ar_valid = model.mem_axi4_0_ar_valid;
    out.ar_id = model.mem_axi4_0_ar_bits_id;
    out.ar_addr = model.mem_axi4_0_ar_bits_addr;
    out.ar_len = model.mem_axi4_0_ar_bits_len;
    out.ar_size = model.mem_axi4_0_ar_bits_size;
    out.r_ready = model.mem_axi4_0_r_ready;
    return out;
  }

  void set_mmio(AxiInputs &in) override {
    model.mmio_axi4_0_aw_ready = in.aw_ready;
    model.mmio_axi4_0_w_ready = in.w_ready;
    model.mmio_axi4_0_b_valid = in.b_valid;
    model.mmio_axi4_0_b_bits_id = in.b_id;
    model.mmio_axi4_0_b_bits_resp = in.b_resp;
    model.mmio_axi4_0_ar_ready = in.ar_ready;
    model.mmio_axi4_0_r_valid = in.r_valid;
    model.mmio_axi4_0_r_bits_id = in.r_id;
    model.mmio_axi4_0_r_bits_data = in.r_data;
    model.mmio_axi4_0_r_bits_resp = in.r_resp;
    model.mmio_axi4_0_r_bits_last = in.r_last;
  }

  AxiOutputs get_mmio() override {
    AxiOutputs out;
    out.aw_valid = model.mmio_axi4_0_aw_valid;
    out.aw_id = model.mmio_axi4_0_aw_bits_id;
    out.aw_addr = model.mmio_axi4_0_aw_bits_addr;
    out.aw_len = model.mmio_axi4_0_aw_bits_len;
    out.aw_size = model.mmio_axi4_0_aw_bits_size;
    out.w_valid = model.mmio_axi4_0_w_valid;
    out.w_data = model.mmio_axi4_0_w_bits_data;
    out.w_strb = model.mmio_axi4_0_w_bits_strb;
    out.w_last = model.mmio_axi4_0_w_bits_last;
    out.b_ready = model.mmio_axi4_0_b_ready;
    out.ar_valid = model.mmio_axi4_0_ar_valid;
    out.ar_id = model.mmio_axi4_0_ar_bits_id;
    out.ar_addr = model.mmio_axi4_0_ar_bits_addr;
    out.ar_len = model.mmio_axi4_0_ar_bits_len;
    out.ar_size = model.mmio_axi4_0_ar_bits_size;
    out.r_ready = model.mmio_axi4_0_r_ready;
    return out;
  }
};
} // namespace

std::unique_ptr<BoomModel> makeVerilatorModel() {
  return std::make_unique<VerilatorBoomModel>();
}
