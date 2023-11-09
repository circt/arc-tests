CXXFLAGS = -O3 -Wall -std=c++17 -no-pie
VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

REPO_ROOT := $(mkfile_dir)..

ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
BINARY ?= $(REPO_ROOT)/benchmarks/dhrystone.riscv
CONFIG ?= small

SOURCE_DIR ?= $(mkfile_dir)
BUILD_DIR ?= $(REPO_ROOT)/build/rocket-$(CONFIG)
$(shell mkdir -p $(BUILD_DIR))

SOURCE_MODEL ?= $(SOURCE_DIR)/rocket
BUILD_MODEL ?= $(BUILD_DIR)/rocket

ARCILATOR_ARGS ?= --mlir-timing --print-debug-info
VERILATOR_ARGS ?= -DPRINTF_COND=0 -DASSERT_VERBOSE_COND=0 -DSTOP_COND=0

TRACE ?= 0
RUN_ESSENT ?= 0
RUN_ARC ?= 1
RUN_VTOR ?= 1

ifeq ($(RUN_ESSENT),1)
	CXXFLAGS += -DRUN_ESSENT
endif
ifeq ($(RUN_ARC),1)
	CXXFLAGS += -DRUN_ARC
endif
ifeq ($(RUN_VTOR),1)
	CXXFLAGS += -DRUN_VTOR
endif

ifeq ($(TRACE),1)
	ARCILATOR_ARGS += --observe-wires --observe-ports --observe-named-values
	VERILATOR_ARGS += --trace --trace-underscore
	CXXFLAGS += -DTRACE
else
	ARCILATOR_ARGS += --observe-wires=0 --observe-ports=0 --observe-named-values=0
endif

#===-------------------------------------------------------------------------===
# FIRRTL to HW
#===-------------------------------------------------------------------------===

$(BUILD_MODEL).fir: $(SOURCE_MODEL)-$(CONFIG).fir.gz
	gzip -dc $< > $@

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
	verilator --public -sv -cc -Mdir $(BUILD_MODEL)-vtor $^ --build -j 0 -O3 -Wno-WIDTH -CFLAGS -DVL_TIME_CONTEXT $(VERILATOR_ARGS)
	cp $(BUILD_MODEL)-vtor/Vrocket__ALL.a $(BUILD_MODEL)-vtor.a
	cp $(BUILD_MODEL)-vtor/Vrocket.h $(BUILD_MODEL)-vtor.h

#===-------------------------------------------------------------------------===
# Essent
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-essent.h: $(BUILD_MODEL).fir
	cd $(REPO_ROOT)/essent && sbt assembly
	$(REPO_ROOT)/essent/utils/bin/essent -O3 --part-cutoff=8 --parallel=1 $(BUILD_MODEL).fir
	mv $(BUILD_DIR)/RocketSystem.h $(BUILD_MODEL)-essent.h

#===-------------------------------------------------------------------------===
# Testbench
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-model-arc.o: $(SOURCE_MODEL)-model-arc.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-arc.h
	$(CXX) $(CXXFLAGS) -g -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -c $< -o $@

$(BUILD_MODEL)-model-vtor.o: $(SOURCE_MODEL)-model-vtor.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor.h
	$(CXX) $(CXXFLAGS) -g -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@

$(BUILD_MODEL)-model-essent.o: $(SOURCE_MODEL)-model-essent.cpp $(BUILD_MODEL)-essent.h $(SOURCE_MODEL)-model.h
	$(CXX) $(CXXFLAGS) -g -I$(BUILD_DIR) -I$(REPO_ROOT)/essent-utils -fbracket-depth=1024 -c $< -o $@

MAIN_DEP_LIST := $(SOURCE_MODEL)-main.cpp
ifeq ($(RUN_ESSENT),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-essent.o
endif
ifeq ($(RUN_ARC),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-arc.o $(BUILD_MODEL)-arc.o
endif
ifeq ($(RUN_VTOR),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-vtor.o $(BUILD_MODEL)-vtor.a $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp $(VERILATOR_ROOT)/include/verilated_threads.cpp
endif

$(BUILD_MODEL)-main: $(MAIN_DEP_LIST)
	$(CXX) $(CXXFLAGS) -g -latomic -I$(REPO_ROOT)/elfio $^ -o $@

#===-------------------------------------------------------------------------===
# Convenience
#===-------------------------------------------------------------------------===

run: $(BUILD_MODEL)-main
	$(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

run-arcs: RUN_ARGS += --arcs
run-arcs: run
run-vtor: RUN_ARGS += --vtor
run-vtor: run
run-essent: RUN_ARGS += --essent
run-essent: run
run-trace: RUN_ARGS += --trace $(BUILD_MODEL).vcd
run-trace: run

benchmark: $(BUILD_MODEL)-main
	$(REPO_ROOT)/benchmark.py -- $(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

benchmark-arcs: RUN_ARGS += --arcs
benchmark-arcs: benchmark
benchmark-vtor: RUN_ARGS += --vtor
benchmark-vtor: benchmark

hyperfine: $(BUILD_MODEL)-main
	hyperfine --show-output --warmup=3 "$(BUILD_MODEL)-main $(BINARY) --arcs" "$(BUILD_MODEL)-main $(BINARY) --vtor"
