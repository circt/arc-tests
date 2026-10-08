VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)
ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
REPO_ROOT := ..
BINARY ?= $(REPO_ROOT)/benchmarks/dhrystone_rv32i.riscv

ARCRUNTIME ?= 0
ARCRUNTIME_INCLUDE_DIR   ?= $(ARCILATOR_UTILS_ROOT)/../../include
ARCRUNTIME_CIRCT_LIB_DIR ?= $(ARCILATOR_UTILS_ROOT)/../lib
ARCRUNTIME_LLVM_LIB_DIR  ?= $(dir $(shell which llc))/../lib
ARCRUNTIME_LINK_LIBS     := -lCIRCTArcRuntime -lLLVMSupport -lLLVMDemangle

CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS =
ifeq ($(shell uname), Linux)
	CXXFLAGS += -no-pie
	LDFLAGS += -latomic
endif

BUILD_DIR ?= build
$(shell mkdir -p $(BUILD_DIR))

SOURCE_MODEL ?= snitch
BUILD_MODEL ?= $(BUILD_DIR)/snitch

ARCILATOR_ARGS ?= --mlir-timing --mlir-pass-statistics
VERILATOR_ARGS ?= -DPRINTF_COND=0 -DASSERT_VERBOSE_COND=0 -DSTOP_COND=0 -Wno-ASCRANGE -Wno-WIDTHCONCAT -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-WIDTHEXPAND

TRACE ?= 0

ifeq ($(TRACE),1)
	ARCILATOR_ARGS += --observe-wires --observe-ports --observe-named-values --observe-registers --observe-memories
	VERILATOR_ARGS += --trace --trace-underscore
	CXXFLAGS += -DTRACE
else
	ARCILATOR_ARGS += --observe-wires=0 --observe-ports=0 --observe-named-values=0 --observe-registers=0 --observe-memories=0
endif

ifeq ($(ARCRUNTIME),1)
	ARCRUNTIME_CXXFLAGS  = -DARC_USE_COMPILED_RUNTIME_LIB
	ARCRUNTIME_CXXFLAGS += -I$(ARCRUNTIME_INCLUDE_DIR)
	ARCRUNTIME_LD_FLAGS  = -L$(ARCRUNTIME_CIRCT_LIB_DIR)
	ARCRUNTIME_LD_FLAGS += -L$(ARCRUNTIME_LLVM_LIB_DIR)
	ARCRUNTIME_LD_FLAGS += $(ARCRUNTIME_LINK_LIBS)
endif

#===-------------------------------------------------------------------------===
# SystemVerilog to HW
#===-------------------------------------------------------------------------===

$(BUILD_MODEL).mlir: snitch_th.sv snitch.sv
	circt-verilog snitch_th.sv snitch.sv --top=snitch_th -o $@

#===-------------------------------------------------------------------------===
# Arcilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-arc.o $(BUILD_MODEL).json &: $(BUILD_MODEL).mlir
	arcilator $< --state-file=$(BUILD_MODEL).json -o $(BUILD_MODEL)-arc.ll $(ARCILATOR_ARGS)
	opt -O3 -S $(BUILD_MODEL)-arc.ll | llc -O3 --filetype=obj -o $(BUILD_MODEL)-arc.o

$(BUILD_MODEL)-arc.h: $(BUILD_MODEL).json
	python3 $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@

#===-------------------------------------------------------------------------===
# Verilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-vtor.a $(BUILD_MODEL)-vtor.h &: snitch_th.sv snitch.sv
	verilator -O3 --noassert --x-assign fast --x-initial fast -top snitch_th -prefix VSnitch -sv -cc -Mdir $(BUILD_MODEL)-vtor $^ --build -j 0 $(VERILATOR_ARGS)
	cp $(BUILD_MODEL)-vtor/VSnitch__ALL.a $(BUILD_MODEL)-vtor.a
	cp $(BUILD_MODEL)-vtor/VSnitch.h $(BUILD_MODEL)-vtor.h

#===-------------------------------------------------------------------------===
# Verilator CIRCT
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-circt.sv: $(BUILD_MODEL).mlir
	firtool --lowering-options=disallowLocalVariables $< -o $@

$(BUILD_MODEL)-vtor-circt.a $(BUILD_MODEL)-vtor-circt.h &: $(BUILD_MODEL)-circt.sv $(REPO_ROOT)/EICG_wrapper.v
	verilator -O3 --noassert --x-assign fast --x-initial fast -top snitch_th -prefix VSnitchCirct -Idesign -sv -cc -Mdir $(BUILD_MODEL)-vtor-circt $^ --build -j 0 $(VERILATOR_ARGS)
	cp $(BUILD_MODEL)-vtor-circt/VSnitchCirct__ALL.a $(BUILD_MODEL)-vtor-circt.a
	cp $(BUILD_MODEL)-vtor-circt/VSnitchCirct.h $(BUILD_MODEL)-vtor-circt.h

#===-------------------------------------------------------------------------===
# Testbench
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-model-arc.o: $(SOURCE_MODEL)-model-arc.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-arc.h
	$(CXX) $(CXXFLAGS) $(ARCRUNTIME_CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -c $< -o $@

$(BUILD_MODEL)-model-vtor.o: $(SOURCE_MODEL)-model-vtor.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor.h
	$(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@

$(BUILD_MODEL)-model-vtor-circt.o: $(SOURCE_MODEL)-model-vtor-circt.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor-circt.h
	$(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@

$(BUILD_MODEL)-main: $(SOURCE_MODEL)-main.cpp $(BUILD_MODEL)-model-arc.o $(BUILD_MODEL)-arc.o $(BUILD_MODEL)-model-vtor.o $(BUILD_MODEL)-vtor.a $(BUILD_MODEL)-model-vtor-circt.o $(BUILD_MODEL)-vtor-circt.a $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp $(VERILATOR_ROOT)/include/verilated_threads.cpp
	$(CXX) $(CXXFLAGS) $(ARCRUNTIME_CXXFLAGS) -g $(LDFLAGS) -I$(REPO_ROOT)/elfio $^ -o $@ $(ARCRUNTIME_LD_FLAGS)

#===-------------------------------------------------------------------------===
# Convenience
#===-------------------------------------------------------------------------===

run: $(BUILD_MODEL)-main
	$(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

run-arcs: RUN_ARGS += --arcs
run-arcs: run
run-vtor: RUN_ARGS += --vtor
run-vtor: run
run-vtor-circt: RUN_ARGS += --vtor-circt
run-vtor-circt: run
run-trace: RUN_ARGS += --trace $(BUILD_MODEL).vcd
run-trace: run

benchmark: $(BUILD_MODEL)-main
	$(REPO_ROOT)/benchmark.py -- $(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

benchmark-arcs: RUN_ARGS += --arcs
benchmark-arcs: benchmark
benchmark-vtor: RUN_ARGS += --vtor
benchmark-vtor: benchmark
benchmark-vtor-circt: RUN_ARGS += --vtor-circt
benchmark-vtor-circt: benchmark

hyperfine: $(BUILD_MODEL)-main
	hyperfine --warmup=3 "$(BUILD_MODEL)-main $(BINARY) --arcs" "$(BUILD_MODEL)-main $(BINARY) --vtor"
