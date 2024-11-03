#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// Memory signals going into the design.
struct AxiInputs {
  bool data_qready_i = false;
  bool data_perror_i = false;
  bool data_pvalid_i = false;
  size_t data_pdata_i = false;

  size_t inst_data_i = 0;
  bool inst_ready_i = false;
};

/// Memory signals coming out of the design.
struct AxiOutputs {
  size_t data_qaddr_o = 0;
  bool data_qwrite_o = false;
  size_t data_qamo_o = 0;
  size_t data_qdata_o = 0;
  size_t data_qstrb_o = 0;
  bool data_qvalid_o = false;
  bool data_pready_o = false;

  size_t inst_addr_o = 0;
  bool inst_valid_o = false;
};

/// Abstract interface to an Arcilator or Verilator model.
class SnitchModel {
public:
  static constexpr const char *PORT_NAMES[] = {
#define PORT(name) #name,
#include "ports.def"
  };
  static constexpr size_t NUM_PORTS = sizeof(PORT_NAMES) / sizeof(*PORT_NAMES);
  using Ports = std::array<uint64_t, NUM_PORTS>;

  SnitchModel() {}
  virtual ~SnitchModel();

  virtual void vcd_start(const char *outputFile) {}
  virtual void vcd_dump(size_t cycle) {}
  virtual void eval() {}
  virtual Ports get_ports() { return {}; }
  virtual void set_clock(bool clock) {}
  virtual void set_reset(bool reset) {}
  virtual void set_mem(AxiInputs &in) {}
  virtual AxiOutputs get_mem() { return {}; }

  const char *name = "unknown";
  std::chrono::high_resolution_clock::duration duration =
      std::chrono::high_resolution_clock::duration::zero();
};

std::unique_ptr<SnitchModel> makeArcilatorModel();
std::unique_ptr<SnitchModel> makeVerilatorModel();
std::unique_ptr<SnitchModel> makeVerilatorCirctModel();
