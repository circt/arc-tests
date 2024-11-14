#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// Signals for the instruction interface.
struct Inst {
  bool valid_o;
  uint32_t addr_o;
  bool ready_i;
  uint32_t data_i;
};

/// Signals for the data interface.
struct Data {
  bool valid_o;
  uint32_t addr_o;
  bool write_o;
  uint64_t wdata_o;
  uint8_t wstrb_o;
  bool ready_i;
  uint64_t rdata_i;
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

  virtual void vcd_start(const char *outputFile) = 0;
  virtual void vcd_dump(size_t cycle) = 0;
  virtual void eval() = 0;
  virtual Ports get_ports() { return {}; }
  virtual void set_clock(bool clock) = 0;
  virtual void set_reset(bool reset) = 0;
  virtual void set_inst(const Inst &in) = 0;
  virtual void get_inst(Inst &out) = 0;
  virtual void set_data(const Data &in) = 0;
  virtual void get_data(Data &out) = 0;

  const char *name = "unknown";
  std::chrono::high_resolution_clock::duration duration =
      std::chrono::high_resolution_clock::duration::zero();
};

std::unique_ptr<SnitchModel> makeArcilatorModel();
std::unique_ptr<SnitchModel> makeVerilatorModel();
std::unique_ptr<SnitchModel> makeVerilatorCirctModel();
