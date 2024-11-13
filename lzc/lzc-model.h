#pragma once

#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/// Abstract interface to an Arcilator or Verilator model.
class LzcModel {
public:
  static constexpr const char *PORT_NAMES[] = {
#define PORT(name) #name,
#include "ports.def"
  };
  static constexpr size_t NUM_PORTS = sizeof(PORT_NAMES) / sizeof(*PORT_NAMES);
  using Ports = std::array<uint64_t, NUM_PORTS>;

  LzcModel() {}
  virtual ~LzcModel();

  virtual void vcd_start(const char *outputFile) = 0;
  virtual void vcd_dump(size_t cycle) = 0;
  virtual void eval() = 0;
  virtual Ports get_ports() { return {}; }
  virtual uint32_t getCount() = 0;
  virtual bool getEmpty() = 0;
  virtual void setIn(uint32_t in) = 0;

  const char *name = "unknown";
  std::chrono::high_resolution_clock::duration duration =
      std::chrono::high_resolution_clock::duration::zero();
};

std::unique_ptr<LzcModel> makeArcilatorModel();
std::unique_ptr<LzcModel> makeVerilatorModel();
std::unique_ptr<LzcModel> makeVerilatorCirctModel();
