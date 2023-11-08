#include "rocket-essent.h"
#include "rocket-model.h"
#include <fstream>
#include <optional>

namespace {
class EssentRocketModel : public RocketModel {
  RocketSystem model;

public:
  EssentRocketModel() { name = "essent"; }

  void vcd_start(const char *outputFile) override {
#ifdef TRACE
    model.genWaveHeader();
#endif
  }

  void vcd_dump(size_t cycle) override { }

  void eval(bool update = false) override {
    model.eval(update, /*verbose=*/ true, !model.reset);
  }

  Ports get_ports() override {
    return {
#define PORT(name) model.name.as_single_word(),
#include "ports.def"
    };
  }

  void set_reset(bool reset) override {
    // clang-format off
    model.reset = reset;
    // clang-format on
  }

  void set_clock(bool clock) override {
    // clang-format off
    model.clock = clock;
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
    out.aw_valid = model.mem_axi4_0_aw_valid.as_single_word();
    out.aw_id = model.mem_axi4_0_aw_bits_id.as_single_word();
    out.aw_addr = model.mem_axi4_0_aw_bits_addr.as_single_word();
    out.aw_len = model.mem_axi4_0_aw_bits_len.as_single_word();
    out.aw_size = model.mem_axi4_0_aw_bits_size.as_single_word();
    out.w_valid = model.mem_axi4_0_w_valid.as_single_word();
    out.w_data = model.mem_axi4_0_w_bits_data.as_single_word();
    out.w_strb = model.mem_axi4_0_w_bits_strb.as_single_word();
    out.w_last = model.mem_axi4_0_w_bits_last.as_single_word();
    out.b_ready = model.mem_axi4_0_b_ready.as_single_word();
    out.ar_valid = model.mem_axi4_0_ar_valid.as_single_word();
    out.ar_id = model.mem_axi4_0_ar_bits_id.as_single_word();
    out.ar_addr = model.mem_axi4_0_ar_bits_addr.as_single_word();
    out.ar_len = model.mem_axi4_0_ar_bits_len.as_single_word();
    out.ar_size = model.mem_axi4_0_ar_bits_size.as_single_word();
    out.r_ready = model.mem_axi4_0_r_ready.as_single_word();
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
    out.aw_valid = model.mmio_axi4_0_aw_valid.as_single_word();
    out.aw_id = model.mmio_axi4_0_aw_bits_id.as_single_word();
    out.aw_addr = model.mmio_axi4_0_aw_bits_addr.as_single_word();
    out.aw_len = model.mmio_axi4_0_aw_bits_len.as_single_word();
    out.aw_size = model.mmio_axi4_0_aw_bits_size.as_single_word();
    out.w_valid = model.mmio_axi4_0_w_valid.as_single_word();
    out.w_data = model.mmio_axi4_0_w_bits_data.as_single_word();
    out.w_strb = model.mmio_axi4_0_w_bits_strb.as_single_word();
    out.w_last = model.mmio_axi4_0_w_bits_last.as_single_word();
    out.b_ready = model.mmio_axi4_0_b_ready.as_single_word();
    out.ar_valid = model.mmio_axi4_0_ar_valid.as_single_word();
    out.ar_id = model.mmio_axi4_0_ar_bits_id.as_single_word();
    out.ar_addr = model.mmio_axi4_0_ar_bits_addr.as_single_word();
    out.ar_len = model.mmio_axi4_0_ar_bits_len.as_single_word();
    out.ar_size = model.mmio_axi4_0_ar_bits_size.as_single_word();
    out.r_ready = model.mmio_axi4_0_r_ready.as_single_word();
    return out;
  }
};
} // namespace

std::unique_ptr<RocketModel> makeEssentModel() {
  return std::make_unique<EssentRocketModel>();
}
