#include "largeBoom.h"
#include "elfio/elfio.hpp"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>

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

static void advanceAllClocks(void *state) {
  DigitalTop_clock(state);
  DigitalTop_clock_0(state);
  DigitalTop_clock_1(state);
  DigitalTop_clock_2(state);
  DigitalTop_clock_3(state);
  DigitalTop_clock_4(state);
  DigitalTop_clock_5(state);
  DigitalTop_clock_6(state);
  DigitalTop_clock_7(state);
  DigitalTop_clock_8(state);
}

int main(int argc, char **argv) {
  //===--------------------------------------------------------------------===//
  // Process CLI arguments
  //===--------------------------------------------------------------------===//

  if (argc != 2) {
    std::cerr << "usage: " << argv[0] << " PROGRAM_ELF\n";
    return 1;
  }

  //===--------------------------------------------------------------------===//
  // Read ELF into memory
  //===--------------------------------------------------------------------===//

  std::map<uint32_t, uint64_t> memory;
  {
    ELFIO::elfio elf;
    if (!elf.load(argv[1])) {
      std::cerr << "unable to open file " << argv[1] << std::endl;
      return 1;
    }
    std::cerr << std::hex;
    for (const auto &segment : elf.segments) {
      if (segment->get_type() != ELFIO::PT_LOAD ||
          segment->get_memory_size() == 0)
        continue;
      std::cerr << "loading segment at " << segment->get_physical_address()
                << " (virtual address " << segment->get_virtual_address()
                << ")\n";
      for (unsigned i = 0; i < segment->get_memory_size(); ++i) {
        uint32_t addr = segment->get_physical_address() + i;
        uint8_t data = 0;
        if (i < segment->get_file_size())
          data = segment->get_data()[i];
        auto &slot = memory[addr / 4 * 4];
        slot &= ~(0xFF << ((addr % 4) * 8));
        slot |= data << ((addr % 4) * 8);
      }
    }
    std::cerr << std::dec;
    std::cerr << "loaded " << memory.size() * 4 << " program bytes\n";
  }

  //===--------------------------------------------------------------------===//
  // Model initialization and reset
  //===--------------------------------------------------------------------===//

  DigitalTop model;
  auto &dut = model.view;
  void *statePtr = static_cast<void *>(dut.state);

  DigitalTop_passthrough(statePtr);
  dut.reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_implicit_clock_reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_cbus_0_reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_mbus_0_reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_fbus_0_reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_pbus_0_reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_sbus_1_reset = 1;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_sbus_0_reset = 1;
  DigitalTop_passthrough(statePtr);

  unsigned cycle = 0;
  for (unsigned i = 0; i < 100; ++i) {
    ++cycle;
    advanceAllClocks(statePtr);
    DigitalTop_passthrough(statePtr);
  }

  auto duration = std::chrono::high_resolution_clock::duration::zero();
  dut.reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_implicit_clock_reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_cbus_0_reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_mbus_0_reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_fbus_0_reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_pbus_0_reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_sbus_1_reset = 0;
  dut.auto_prci_ctrl_domain_tileResetSetter_clock_in_member_allClocks_subsystem_sbus_0_reset = 0;

  //===--------------------------------------------------------------------===//
  // Simulation loop
  //===--------------------------------------------------------------------===//

  // Memory to Rocket AXI4 interconnect signals
  auto &aw_ready = dut.mem_axi4_0_aw_ready; // i1
  auto &w_ready = dut.mem_axi4_0_w_ready; // i1
  auto &b_valid = dut.mem_axi4_0_b_valid; // i1
  auto &b_bits_id = dut.mem_axi4_0_b_bits_id; // i4
  auto &b_bits_resp = dut.mem_axi4_0_b_bits_resp; // i2
  auto &ar_ready = dut.mem_axi4_0_ar_ready; // i1
  auto &r_valid = dut.mem_axi4_0_r_valid; // i1
  auto &r_bits_id = dut.mem_axi4_0_r_bits_id; // i4
  auto &r_bits_data = dut.mem_axi4_0_r_bits_data; // i64
  auto &r_bits_resp = dut.mem_axi4_0_r_bits_resp; // i2
  auto &r_bits_last = dut.mem_axi4_0_r_bits_last; // i1

  // Rocket to Memory AXI4 interconnect signals
  auto &aw_valid = dut.mem_axi4_0_aw_valid; // i1
  auto &aw_bits_id = dut.mem_axi4_0_aw_bits_id; // i4
  auto &aw_bits_addr = dut.mem_axi4_0_aw_bits_addr; // i32
  auto &aw_bits_len = dut.mem_axi4_0_aw_bits_len; // i8
  auto &aw_bits_size = dut.mem_axi4_0_aw_bits_size; // i3
  auto &aw_bits_burst = dut.mem_axi4_0_aw_bits_burst; // i2
  auto &aw_bits_lock = dut.mem_axi4_0_aw_bits_lock; // i1
  auto &aw_bits_cache = dut.mem_axi4_0_aw_bits_cache; // i4
  auto &aw_bits_prot = dut.mem_axi4_0_aw_bits_prot; // i3
  auto &aw_bits_qos = dut.mem_axi4_0_aw_bits_qos; // i4
  auto &w_valid = dut.mem_axi4_0_w_valid; // i1
  auto &w_bits_data = dut.mem_axi4_0_w_bits_data; // i64
  auto &w_bits_strb = dut.mem_axi4_0_w_bits_strb; // i8
  auto &w_bits_last = dut.mem_axi4_0_w_bits_last; // i1
  auto &b_ready = dut.mem_axi4_0_b_ready; // i1
  auto &ar_valid = dut.mem_axi4_0_ar_valid; // i1
  auto &ar_bits_id = dut.mem_axi4_0_ar_bits_id; // i4
  auto &ar_bits_addr = dut.mem_axi4_0_ar_bits_addr; // i32
  auto &ar_bits_len = dut.mem_axi4_0_ar_bits_len; // i8
  auto &ar_bits_size = dut.mem_axi4_0_ar_bits_size; // i3
  auto &ar_bits_burst = dut.mem_axi4_0_ar_bits_burst; // i2
  auto &ar_bits_lock = dut.mem_axi4_0_ar_bits_lock; // i1
  auto &ar_bits_cache = dut.mem_axi4_0_ar_bits_cache; // i4
  auto &ar_bits_prot = dut.mem_axi4_0_ar_bits_prot; // i3
  auto &ar_bits_qos = dut.mem_axi4_0_ar_bits_qos; // i4
  auto &r_ready = dut.mem_axi4_0_r_ready; // i1

  enum {
    RESP_OKAY = 0b00,
    RESP_EXOKAY = 0b01,
    RESP_SLVERR = 0b10,
    RESP_DECERR = 0b11
  };

  uint32_t next_raddr;
  uint32_t next_waddr;
  bool per_next_read = false;
  bool per_next_write = false;
  uint8_t next_rid;
  uint8_t next_bid;
  bool next_bvalid = false;

  for (unsigned i = 0; i < 1000000; ++i) {
    ++cycle;

    // Advance model to next cycle
    auto t_before = std::chrono::high_resolution_clock::now();
    advanceAllClocks(statePtr);
    DigitalTop_passthrough(statePtr);
    auto t_after = std::chrono::high_resolution_clock::now();
    duration += t_after - t_before;

    // Handle AXI memory interconnect
    if (per_next_read) {
      r_valid = true;
      auto it = memory.find(next_raddr);
      if (it != memory.end())
        r_bits_data = it->second;
      else
        r_bits_data = 0x10500073;
      r_bits_id = next_rid;
      r_bits_resp = RESP_OKAY;
      r_bits_last = true;
    }

    if (next_bvalid) {
      b_valid = true;
      b_bits_id = next_bid;
      b_bits_resp = RESP_OKAY;
    }

    if (b_ready) {
      next_bvalid = false;
      b_valid = false;
    }

    w_ready = false;

    if (per_next_write && w_valid) {
      w_ready = true;
      assert(w_bits_strb == 256);
      memory[next_waddr] = w_bits_data;
      assert(w_bits_last == 1);
      next_bvalid = true;
    }

    aw_ready = 0;
    ar_ready = 0;
    per_next_write = false;

    if (r_ready) {
      per_next_read = false;
      r_valid = false;
    }

    if (aw_valid) {
      assert(aw_bits_len == 1);
      aw_ready = 1;
      next_waddr = aw_bits_addr;
      per_next_write = true;
      next_bid = aw_bits_id;
    }
    if (ar_valid) {
      assert(ar_bits_len == 1);
      ar_ready = 1;
      next_raddr = ar_bits_addr;
      per_next_read = true;
      next_rid = ar_bits_id;
    }

    // Emit progress indication
    if (cycle % 10000 == 0) {
      auto seconds =
          std::chrono::duration_cast<std::chrono::duration<double>>(duration)
              .count();
      std::cerr << "Cycle " << cycle << " [" << (i / seconds) << " Hz]\n";
    }
  }

  return 0;
}
