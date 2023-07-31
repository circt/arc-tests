#include "boom-arc.h"
#include "boom-model.h"
#include <fstream>
#include <optional>

extern "C" {
void DigitalTop_clock(void *state);
void DigitalTop_clock_0(void *state);
void DigitalTop_clock_1(void *state);
void DigitalTop_clock_2(void *state);
void DigitalTop_clock_3(void *state);
void DigitalTop_clock_4(void *state);
void DigitalTop_clock_5(void *state);
void DigitalTop_clock_6(void *state);
void DigitalTop_clock_7(void *state);
void DigitalTop_clock_8(void *state);
}

namespace {
class ArcilatorBoomModel : public BoomModel {
  DigitalTop model;
  std::ofstream vcd_stream;
  std::unique_ptr<ValueChangeDump<DigitalTopLayout>> model_vcd;

public:
  ArcilatorBoomModel() { name = "arcs"; }

  void vcd_start(const char *outputFile) override {
    vcd_stream.open(outputFile);
    model_vcd.reset(
        new ValueChangeDump<DigitalTopLayout>(model.vcd(vcd_stream)));
  }

  void vcd_dump(size_t cycle) override {
    if (model_vcd) {
      model_vcd->time = cycle;
      model_vcd->writeTimestep(0);
    }
  }

  void clock() override {
    DigitalTop_clock(&model.storage[0]);
    // DigitalTop_clock_0(&model.storage[0]);
    // DigitalTop_clock_1(&model.storage[0]);
    DigitalTop_clock_2(&model.storage[0]);
    // DigitalTop_clock_3(&model.storage[0]);
    // DigitalTop_clock_4(&model.storage[0]);
    // DigitalTop_clock_5(&model.storage[0]);
    // DigitalTop_clock_6(&model.storage[0]);
    // DigitalTop_clock_7(&model.storage[0]);
    // DigitalTop_clock_8(&model.storage[0]);
    DigitalTop_passthrough(&model.storage[0]);
  }

  void passthrough() override { DigitalTop_passthrough(&model.storage[0]); }

  Ports get_ports() override {
    return {
#define PORT(name) model.view.name,
#include "ports.def"
    };
  }

  void set_reset(bool reset) override {
    // clang-format off
    model.view.reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_implicit_clock_reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_cbus_0_reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_mbus_0_reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_fbus_0_reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_pbus_0_reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_sbus_1_reset = reset;
    // model.view.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_sbus_0_reset = reset;
    // clang-format on
  }

  void set_mem(AxiInputs &in) override {
    model.view.mem_axi4_0_aw_ready = in.aw_ready;
    model.view.mem_axi4_0_w_ready = in.w_ready;
    model.view.mem_axi4_0_b_valid = in.b_valid;
    model.view.mem_axi4_0_b_bits_id = in.b_id;
    model.view.mem_axi4_0_b_bits_resp = in.b_resp;
    model.view.mem_axi4_0_ar_ready = in.ar_ready;
    model.view.mem_axi4_0_r_valid = in.r_valid;
    model.view.mem_axi4_0_r_bits_id = in.r_id;
    model.view.mem_axi4_0_r_bits_data = in.r_data;
    model.view.mem_axi4_0_r_bits_resp = in.r_resp;
    model.view.mem_axi4_0_r_bits_last = in.r_last;
  }

  AxiOutputs get_mem() override {
    AxiOutputs out;
    out.aw_valid = model.view.mem_axi4_0_aw_valid;
    out.aw_id = model.view.mem_axi4_0_aw_bits_id;
    out.aw_addr = model.view.mem_axi4_0_aw_bits_addr;
    out.aw_len = model.view.mem_axi4_0_aw_bits_len;
    out.aw_size = model.view.mem_axi4_0_aw_bits_size;
    out.w_valid = model.view.mem_axi4_0_w_valid;
    out.w_data = model.view.mem_axi4_0_w_bits_data;
    out.w_strb = model.view.mem_axi4_0_w_bits_strb;
    out.w_last = model.view.mem_axi4_0_w_bits_last;
    out.b_ready = model.view.mem_axi4_0_b_ready;
    out.ar_valid = model.view.mem_axi4_0_ar_valid;
    out.ar_id = model.view.mem_axi4_0_ar_bits_id;
    out.ar_addr = model.view.mem_axi4_0_ar_bits_addr;
    out.ar_len = model.view.mem_axi4_0_ar_bits_len;
    out.ar_size = model.view.mem_axi4_0_ar_bits_size;
    out.r_ready = model.view.mem_axi4_0_r_ready;
    return out;
  }
};
} // namespace

std::unique_ptr<BoomModel> makeArcilatorModel() {
  return std::make_unique<ArcilatorBoomModel>();
}
