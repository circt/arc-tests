VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))
REPO_ROOT := $(mkfile_dir)..
ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
BINARY ?= $(REPO_ROOT)/benchmarks/dhrystone_rv32i.riscv

CXX_OPT_LEVEL ?= -O3
CXXFLAGS = $(CXX_OPT_LEVEL) -std=c++17 -no-pie
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
AR ?= $(shell which ar)
AR := ${AR}

BUILD_DIR ?= build
$(shell mkdir -p ${mkfile_dir}$(BUILD_DIR))

SOURCE_MODEL ?= lfsr_16bit
BUILD_MODEL ?= ${mkfile_dir}$(BUILD_DIR)/lfsr_16bit

ARCILATOR_ARGS ?= --mlir-timing --mlir-pass-statistics
VERILATOR_ARGS ?= -DPRINTF_COND=0 -DASSERT_VERBOSE_COND=0 -DSTOP_COND=0 -Wno-ASCRANGE -Wno-WIDTHCONCAT -Wno-CASEINCOMPLETE -Wno-LATCH -Wno-UNOPTFLAT -Wno-WIDTHEXPAND

TRACE ?= 0
RUN_ARC ?= 1
RUN_VTOR ?= 1
RUN_VTOR_CIRCT ?= 1

BENCHMARK_MODE ?= 0
BENCHMARK_OUT_FILE ?= ${mkfile_dir}/$(BUILD_DIR)/measurements-A$(RUN_ARC)V$(RUN_VTOR)C$(RUN_VTOR_CIRCT)-$(shell date -u +"%Y-%m-%dT%H:%M:%SZ").txt
BENCHMARK_OUT_DIR := $(dir $(BENCHMARK_OUT_FILE))
$(shell mkdir -p $(BENCHMARK_OUT_DIR))
BENCHMARK_PREFIX ?=
BENCHMARK_SUFFIX =
ifeq ($(BENCHMARK_MODE),1)
# BENCHMARK_PREFIX += sudo chrt -f 99 perf stat -ddd 
	BENCHMARK_SUFFIX += 2>&1 | tee -a ${mkfile_dir}/$(BENCHMARK_OUT_FILE)
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

$(BUILD_MODEL).mlir: ${mkfile_dir}/design/lfsr_16bit.sv $(CIRCT_VERILOG)
	$(BENCHMARK_PREFIX) $(CIRCT_VERILOG) ${mkfile_dir}/design/lfsr_16bit.sv --top=lfsr_16bit -I./design -o $@ $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Arcilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-arc.o $(BUILD_MODEL).json: $(BUILD_MODEL).mlir $(ARCILATOR)
	$(BENCHMARK_PREFIX) $(ARCILATOR) $< --state-file=$(BUILD_MODEL).json -o $(BUILD_MODEL)-arc.ll $(ARCILATOR_ARGS) $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(OPT) --strip-debug $(CXX_OPT_LEVEL) -S $(BUILD_MODEL)-arc.ll -o $(BUILD_MODEL)-llvm.ll $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(LLC) -O3 --filetype=obj $(BUILD_MODEL)-llvm.ll -o $(BUILD_MODEL)-arc.o $(BENCHMARK_SUFFIX)
# objdump -d $(BUILD_MODEL)-arc.o > $(BUILD_MODEL)-arc.s

$(BUILD_MODEL)-arc.h: $(BUILD_MODEL).json
	python3 $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@

#===-------------------------------------------------------------------------===
# Verilator
#===-------------------------------------------------------------------------===

VERILATOR_CPP_FLAGS = -DVM_COVERAGE=0 -DVM_SC=0 -DVM_TRACE=0 -DVM_TRACE_FST=0 -DVM_TRACE_VCD=0 -faligned-new -fcf-protection=none -Wno-bool-operation -Wno-sign-compare -Wno-uninitialized -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-unused-variable -Wno-shadow -DVL_TIME_CONTEXT

$(BUILD_MODEL)-vtor.a $(BUILD_MODEL)-vtor.h: design/lfsr_16bit.sv
	$(BENCHMARK_PREFIX) $(VERILATOR) -prefix VLfsr_16bit -O3 --noassert --x-assign fast --x-initial fast --threads 1 -sv -cc -Mdir $(BUILD_MODEL)-vtor $^ -j 0 $(VERILATOR_ARGS) $(BENCHMARK_SUFFIX)
	cd $(BUILD_MODEL)-vtor/ && $(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) $(VERILATOR_CPP_FLAGS) -c -I/$(VERILATOR_ROOT)/include *.cpp $(BENCHMARK_SUFFIX)
	cd $(BUILD_MODEL)-vtor/ && $(BENCHMARK_PREFIX) $(AR) rcs $(BUILD_MODEL)-vtor.a *.o $(BENCHMARK_SUFFIX)
	cp $(BUILD_MODEL)-vtor/VLfsr_16bit.h $(BUILD_MODEL)-vtor.h

#===-------------------------------------------------------------------------===
# Verilator CIRCT
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-circt.sv: $(BUILD_MODEL).mlir
	$(BENCHMARK_PREFIX) $(FIRTOOL) --lowering-options=disallowLocalVariables $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-vtor-circt.a $(BUILD_MODEL)-vtor-circt.h: $(BUILD_MODEL)-circt.sv
	$(BENCHMARK_PREFIX) $(VERILATOR) -prefix VLfsr_16bitCirct -O3 --noassert --x-assign fast --x-initial fast --threads 1 -sv -cc -Mdir $(BUILD_MODEL)-vtor-circt $^ -j 0 $(VERILATOR_ARGS) $(BENCHMARK_SUFFIX)
	cd $(BUILD_MODEL)-vtor-circt/ && $(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) $(VERILATOR_CPP_FLAGS) -c -I/$(VERILATOR_ROOT)/include *.cpp $(BENCHMARK_SUFFIX)
	cd $(BUILD_MODEL)-vtor-circt/ && $(BENCHMARK_PREFIX) $(AR) rcs $(BUILD_MODEL)-vtor-circt.a *.o $(BENCHMARK_SUFFIX)
	cp $(BUILD_MODEL)-vtor-circt/VLfsr_16bitCirct.h $(BUILD_MODEL)-vtor-circt.h

#===-------------------------------------------------------------------------===
# Testbench
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-model-arc.o: $(SOURCE_MODEL)-model-arc.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-arc.h
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I${mkfile_dir}/$(BUILD_DIR) -c $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-model-vtor.o: $(SOURCE_MODEL)-model-vtor.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor.h
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -I${mkfile_dir}/$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-model-vtor-circt.o: $(SOURCE_MODEL)-model-vtor-circt.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor-circt.h
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -I${mkfile_dir}/$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@ $(BENCHMARK_SUFFIX)

MAIN_DEP_LIST := $(SOURCE_MODEL)-main.cpp
ifeq ($(RUN_ARC),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-arc.o $(BUILD_MODEL)-arc.o
endif
ifeq ($(RUN_VTOR),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-vtor.o $(BUILD_MODEL)-vtor.a
endif
ifeq ($(RUN_VTOR_CIRCT),1)
	MAIN_DEP_LIST += $(BUILD_MODEL)-model-vtor-circt.o $(BUILD_MODEL)-vtor-circt.a 
endif
#ifeq ($(filter 1, $(RUN_VTOR_CIRCT) $(RUN_VTOR)),1)
  MAIN_DEP_LIST += $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp $(VERILATOR_ROOT)/include/verilated_threads.cpp
#endif

$(BUILD_MODEL)-main: $(MAIN_DEP_LIST)
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) $(LDFLAGS) -I$(REPO_ROOT)/elfio $^ -o $@ $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Convenience
#===-------------------------------------------------------------------------===

binary-size: $(BUILD_MODEL)-main
	ls -al ${mkfile_dir}/$(BUILD_DIR) $(BENCHMARK_SUFFIX)

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
	hyperfine --warmup=3 "$(BUILD_MODEL)-main $(BINARY) --arcs" "$(BUILD_MODEL)-main $(BINARY) --vtor" "$(BUILD_MODEL)-main $(BINARY) --vtor-circt"
