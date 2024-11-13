VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
REPO_ROOT := $(mkfile_dir)..
ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
BINARY ?= $(REPO_ROOT)/benchmarks/dhrystone.riscv

CXX_OPT_LEVEL ?= -O3
CXXFLAGS = $(CXX_OPT_LEVEL) -Wall -std=c++17 -no-pie
LDFLAGS =
ifeq ($(shell uname), Linux)
	CXXFLAGS += -no-pie
	LDFLAGS += -latomic
endif

CIRCT_VERILOG ?= $(shell which circt-verilog)
CIRCT_VERILOG := ${CIRCT_VERILOG}
ARCILATOR ?= $(shell which arcilator)
ARCILATOR := ${ARCILATOR}
VERILATOR ?= $(shell which verilator)
VERILATOR := ${VERILATOR}
FIRTOOL ?= $(shell which firtool)
FIRTOOL := ${FIRTOOL}
OPT ?= $(shell which opt)
OPT := ${OPT}
LLC ?= $(shell which llc)
LLC := ${LLC}

BUILD_DIR ?= build
$(shell mkdir -p $(BUILD_DIR))

SOURCE_MODEL ?= graycode
BUILD_MODEL ?= $(BUILD_DIR)/graycode

ARCILATOR_ARGS ?= --mlir-timing --print-debug-info --mlir-pass-statistics
VERILATOR_ARGS ?= -DPRINTF_COND=0 -DASSERT_VERBOSE_COND=0 -DSTOP_COND=0 -Wno-ASCRANGE -Wno-WIDTHCONCAT -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-UNOPTFLAT

TRACE ?= 0
RUN_ARC ?= 1
RUN_VTOR ?= 1
RUN_VTOR_CIRCT ?= 1

BENCHMARK_MODE ?= 0
BENCHMARK_OUT_FILE ?= $(BUILD_DIR)/measurements-A$(RUN_ARC)V$(RUN_VTOR)C$(RUN_VTOR_CIRCT)-$(shell date --iso=seconds).txt
BENCHMARK_OUT_FILE := ${BENCHMARK_OUT_FILE}
BENCHMARK_PREFIX ?=
BENCHMARK_SUFFIX =
ifeq ($(BENCHMARK_MODE),1)
# BENCHMARK_PREFIX += sudo chrt -f 99 perf stat -ddd 
	BENCHMARK_SUFFIX += >> $(BENCHMARK_OUT_FILE) 2>&1
endif

ifeq ($(RUN_ARC),1)
	CXXFLAGS += -DRUN_ARC
endif
ifeq ($(RUN_VTOR),1)
	CXXFLAGS += -DRUN_VTOR
endif
ifeq ($(RUN_VTOR_CIRCT),1)
	CXXFLAGS += -DRUN_VTOR_CIRCT
endif

ifeq ($(TRACE),1)
	ARCILATOR_ARGS += --observe-wires --observe-ports --observe-named-values --observe-registers --observe-memories
	VERILATOR_ARGS += --trace --trace-underscore
	CXXFLAGS += -DTRACE
else
	ARCILATOR_ARGS += --observe-wires=0 --observe-ports=0 --observe-named-values=0 --observe-registers=0 --observe-memories=0
endif

#===-------------------------------------------------------------------------===
# SystemVerilog to HW
#===-------------------------------------------------------------------------===

start:
	echo "START: Current timestamp in milliseconds: ${shell date +%s%3N}" $(BENCHMARK_SUFFIX)

build/graycode.mlir: design/graycode.sv $(CIRCT_VERILOG) start
	$(BENCHMARK_PREFIX) $(CIRCT_VERILOG) design/graycode.sv --top=graycode -I./design -o $@ --mlir-print-debuginfo $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Arcilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-arc.o $(BUILD_MODEL).json: $(BUILD_MODEL).mlir $(ARCILATOR)
	$(BENCHMARK_PREFIX) $(ARCILATOR) $< --state-file=$(BUILD_MODEL).json -o $(BUILD_MODEL)-arc.ll $(ARCILATOR_ARGS) $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(OPT) -O3 -S $(BUILD_MODEL)-arc.ll -o $(BUILD_MODEL)-llvm.ll $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(LLC) -O3 --filetype=obj $(BUILD_MODEL)-llvm.ll -o $(BUILD_MODEL)-arc.o $(BENCHMARK_SUFFIX)
# objdump -d $(BUILD_MODEL)-arc.o > $(BUILD_MODEL)-arc.s

$(BUILD_MODEL)-arc.h: $(BUILD_MODEL).json
	$(BENCHMARK_PREFIX) python3 $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@ $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Verilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-vtor.a $(BUILD_MODEL)-vtor.h: design/graycode.sv start
	$(BENCHMARK_PREFIX) $(VERILATOR) -sv -cc -Mdir $(BUILD_MODEL)-vtor -top graycode -prefix VGraycode $^ -Idesign --build -j 0 -O3 -Wno-WIDTH $(VERILATOR_ARGS) $(BENCHMARK_SUFFIX)
	cp $(BUILD_MODEL)-vtor/VGraycode__ALL.a $(BUILD_MODEL)-vtor.a
	cp $(BUILD_MODEL)-vtor/VGraycode.h $(BUILD_MODEL)-vtor.h

#===-------------------------------------------------------------------------===
# Verilator CIRCT
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-circt.sv: $(BUILD_MODEL).mlir
	$(BENCHMARK_PREFIX) $(FIRTOOL) --lowering-options=disallowLocalVariables $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-vtor-circt.a $(BUILD_MODEL)-vtor-circt.h: $(BUILD_MODEL)-circt.sv $(REPO_ROOT)/EICG_wrapper.v
	$(BENCHMARK_PREFIX) $(VERILATOR) -sv -cc -Mdir $(BUILD_MODEL)-vtor-circt -top graycode $^ -prefix VGraycodeCirct -Idesign --build -j 0 -O3 -Wno-WIDTH $(VERILATOR_ARGS) $(BENCHMARK_SUFFIX)
	cp $(BUILD_MODEL)-vtor-circt/VGraycodeCirct__ALL.a $(BUILD_MODEL)-vtor-circt.a
	cp $(BUILD_MODEL)-vtor-circt/VGraycodeCirct.h $(BUILD_MODEL)-vtor-circt.h

#===-------------------------------------------------------------------------===
# Testbench
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-model-arc.o: $(SOURCE_MODEL)-model-arc.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-arc.h
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -c $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-model-vtor.o: $(SOURCE_MODEL)-model-vtor.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor.h
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-model-vtor-circt.o: $(SOURCE_MODEL)-model-vtor-circt.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor-circt.h
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@ $(BENCHMARK_SUFFIX)

MAIN_DEP_LIST := $(SOURCE_MODEL)-main.cpp
ifeq ($(RUN_ARC),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-arc.o $(BUILD_MODEL)-arc.o
endif
ifeq ($(RUN_VTOR),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-vtor.o $(BUILD_MODEL)-vtor.a$(BUILD_MODEL)-model-vtor.o $(BUILD_MODEL)-vtor.a
endif
ifeq ($(RUN_VTOR_CIRCT),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-vtor-circt.o $(BUILD_MODEL)-vtor-circt.a 
endif
ifeq ($(filter 1, $(RUN_VTOR_CIRCT) $(RUN_VTOR)),1)
  MAIN_DEP_LIST += $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp $(VERILATOR_ROOT)/include/verilated_threads.cpp
endif

$(BUILD_MODEL)-main: $(MAIN_DEP_LIST)
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) $(LDFLAGS) -I$(REPO_ROOT)/elfio $^ -o $@ $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Convenience
#===-------------------------------------------------------------------------===

binary-size: $(BUILD_MODEL)-main
	ls -al $(BUILD_DIR) $(BENCHMARK_SUFFIX)

build: $(BUILD_MODEL)-main
	echo "END: Current timestamp in milliseconds: ${shell date +%s%3N}" $(BENCHMARK_SUFFIX)

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
run-trace-vtors: RUN_ARGS += --vtor --vtor-circt
run-trace-vtors: run-trace

benchmark: $(BUILD_MODEL)-main
	$(REPO_ROOT)/benchmark.py -- $(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS)

benchmark-arcs: RUN_ARGS += --arcs
benchmark-arcs: benchmark
benchmark-vtor: RUN_ARGS += --vtor
benchmark-vtor: benchmark
benchmark-vtor-circt: RUN_ARGS += --vtor-circt
benchmark-vtor-circt: benchmark

hyperfine: $(BUILD_MODEL)-main
	hyperfine --show-output --warmup=3 "$(BUILD_MODEL)-main $(BINARY) --arcs" "$(BUILD_MODEL)-main $(BINARY) --vtor"
