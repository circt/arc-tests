#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// AXI signals going into the design.
struct AxiInputs {
  bool aw_ready = false;

  bool w_ready = false;

  bool b_valid = false;
  size_t b_id = 0;
  size_t b_resp = 0;

  bool ar_ready = false;

  bool r_valid = false;
  size_t r_id = 0;
  size_t r_data = 0;
  size_t r_resp = 0;
  bool r_last = false;
};

/// AXI signals coming out of the design.
struct AxiOutputs {
  bool aw_valid = false;
  size_t aw_id = 0;
  size_t aw_addr = 0;
  size_t aw_len = 0;
  size_t aw_size = 0;

  bool w_valid = false;
  size_t w_data = 0;
  size_t w_strb = 0;
  bool w_last = false;

  bool b_ready = false;

  bool ar_valid = false;
  size_t ar_id = 0;
  size_t ar_addr = 0;
  size_t ar_len = 0;
  size_t ar_size = 0;

  bool r_ready = false;
};

/// Abstract interface to an Arcilator or Verilator model.
class BoomModel {
public:
  static constexpr const char *PORT_NAMES[] = {
#define PORT(name) #name,
#include "ports.def"
  };
  static constexpr size_t NUM_PORTS = sizeof(PORT_NAMES) / sizeof(*PORT_NAMES);
  using Ports = std::array<uint64_t, NUM_PORTS>;

  BoomModel() {}
  virtual ~BoomModel();

  virtual void vcd_start(const char *outputFile) {}
  virtual void vcd_dump(size_t cycle) {}
  virtual void clock() {}
  virtual void passthrough() {}
  virtual Ports get_ports() { return {}; }
  virtual void set_reset(bool reset) {}
  virtual void set_mem(AxiInputs &in) {}
  virtual AxiOutputs get_mem() { return {}; }

  const char *name = "unknown";
  std::chrono::high_resolution_clock::duration duration =
      std::chrono::high_resolution_clock::duration::zero();
};

std::unique_ptr<BoomModel> makeArcilatorModel();
std::unique_ptr<BoomModel> makeVerilatorModel();
