#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// Abstract interface to an Arcilator or Verilator model.
class Lfsr_16bitModel {
public:
  static constexpr const char *PORT_NAMES[] = {
#define PORT(name) #name,
#include "ports.def"
  };
  static constexpr size_t NUM_PORTS = sizeof(PORT_NAMES) / sizeof(*PORT_NAMES);
  using Ports = std::array<uint64_t, NUM_PORTS>;

  Lfsr_16bitModel() {}
  virtual ~Lfsr_16bitModel();

  virtual void vcd_start(const char *outputFile) = 0;
  virtual void vcd_dump(size_t cycle) = 0;
  virtual void eval(bool advance_clock = false) = 0;
  virtual Ports get_ports() { return {}; }
  virtual uint32_t getOut() = 0;
  virtual void setClock(bool clock) = 0;
  virtual void setReset(bool clock) = 0;
  virtual void setEnable(bool clock) = 0;

  const char *name = "unknown";
  std::chrono::high_resolution_clock::duration duration =
      std::chrono::high_resolution_clock::duration::zero();
  std::chrono::high_resolution_clock::duration clock_time =
      std::chrono::high_resolution_clock::duration::zero();
  std::chrono::high_resolution_clock::duration passthrough_time =
      std::chrono::high_resolution_clock::duration::zero();
};

std::unique_ptr<Lfsr_16bitModel> makeArcilatorModel();
std::unique_ptr<Lfsr_16bitModel> makeVerilatorModel();
std::unique_ptr<Lfsr_16bitModel> makeVerilatorCirctModel();
