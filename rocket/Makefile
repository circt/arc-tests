VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)
mkfile_path := $(dir $(MAKEFILE_LIST))
REPO_ROOT := ..
ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
BINARY ?= $(REPO_ROOT)/benchmarks/dhrystone.riscv
CONFIG ?= small-v1.6

CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS =
ifeq ($(shell uname), Linux)
	CXXFLAGS += -no-pie
	LDFLAGS += -latomic
endif

BUILD_DIR ?= build/$(CONFIG)
$(shell mkdir -p $(BUILD_DIR))

SOURCE_MODEL ?= rocket
BUILD_MODEL ?= $(BUILD_DIR)/rocket

ARCILATOR_ARGS ?= --mlir-timing --print-debug-info --mlir-pass-statistics --async-resets-as-sync
VERILATOR_ARGS ?= -DPRINTF_COND=0 -DASSERT_VERBOSE_COND=0 -DSTOP_COND=0

TRACE ?= 0

ifeq ($(TRACE),1)
	ARCILATOR_ARGS += --observe-wires --observe-ports --observe-named-values --observe-registers --observe-memories
	VERILATOR_ARGS += --trace --trace-underscore
	CXXFLAGS += -DTRACE
else
	ARCILATOR_ARGS += --observe-wires=0 --observe-ports=0 --observe-named-values=0 --observe-registers=0 --observe-memories=0
endif

#===-------------------------------------------------------------------------===
# FIRRTL to HW
#===-------------------------------------------------------------------------===

$(BUILD_MODEL).fir: $(SOURCE_MODEL)-$(CONFIG).fir.gz
	gzip -dc $< | sed 's/printf.*/skip/' > $@

$(BUILD_MODEL).mlir: $(BUILD_MODEL).fir
	firtool --ir-hw $< -o $@

#===-------------------------------------------------------------------------===
# Arcilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-arc.o $(BUILD_MODEL).json &: $(BUILD_MODEL).mlir
	arcilator $< --state-file=$(BUILD_MODEL).json -o $(BUILD_MODEL)-arc.ll $(ARCILATOR_ARGS)
	opt -O3 -S $(BUILD_MODEL)-arc.ll | llc -O3 --filetype=obj -o $(BUILD_MODEL)-arc.o
	objdump -d $(BUILD_MODEL)-arc.o > $(BUILD_MODEL)-arc.s

$(BUILD_MODEL)-arc.h: $(BUILD_MODEL).json
	python3 $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@

#===-------------------------------------------------------------------------===
# Verilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL).sv: $(BUILD_MODEL).mlir
	firtool --lowering-options=disallowLocalVariables $< -o $@

$(BUILD_MODEL)-vtor.a $(BUILD_MODEL)-vtor.h &: $(BUILD_MODEL).sv $(REPO_ROOT)/verilator-stubs.sv
	verilator -sv -cc -Mdir $(BUILD_MODEL)-vtor $^ --build -j 0 -Wno-WIDTH -CFLAGS -DVL_TIME_CONTEXT $(VERILATOR_ARGS)
	cp $(BUILD_MODEL)-vtor/Vrocket__ALL.a $(BUILD_MODEL)-vtor.a
	cp $(BUILD_MODEL)-vtor/Vrocket.h $(BUILD_MODEL)-vtor.h

#===-------------------------------------------------------------------------===
# Testbench
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-model-arc.o: $(SOURCE_MODEL)-model-arc.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-arc.h
	$(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -c $< -o $@

$(BUILD_MODEL)-model-vtor.o: $(SOURCE_MODEL)-model-vtor.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor.h
	$(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@

$(BUILD_MODEL)-main: $(SOURCE_MODEL)-main.cpp $(BUILD_MODEL)-model-arc.o $(BUILD_MODEL)-arc.o $(BUILD_MODEL)-model-vtor.o $(BUILD_MODEL)-vtor.a $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp $(VERILATOR_ROOT)/include/verilated_threads.cpp
	$(CXX) $(CXXFLAGS) -g $(LDFLAGS) -I$(REPO_ROOT)/elfio $^ -o $@ -DVL_TIME_CONTEXT

#===-------------------------------------------------------------------------===
# Convenience
#===-------------------------------------------------------------------------===

run: $(BUILD_MODEL)-main
	$(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

run-arcs: RUN_ARGS += --arcs
run-arcs: run
run-vtor: RUN_ARGS += --vtor
run-vtor: run
run-trace: RUN_ARGS += --trace $(BUILD_MODEL).vcd
run-trace: run

benchmark: $(BUILD_MODEL)-main
	$(REPO_ROOT)/benchmark.py -- $(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

benchmark-arcs: RUN_ARGS += --arcs
benchmark-arcs: benchmark
benchmark-vtor: RUN_ARGS += --vtor
benchmark-vtor: benchmark

hyperfine: $(BUILD_MODEL)-main
	hyperfine --warmup=3 "$(BUILD_MODEL)-main $(BINARY) --arcs" "$(BUILD_MODEL)-main $(BINARY) --vtor"
