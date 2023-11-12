CXX_OPT_LEVEL ?= -O3
CXXFLAGS = $(CXX_OPT_LEVEL) -Wall -std=c++17 -no-pie
VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)
ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))

mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

REPO_ROOT := $(mkfile_dir)..

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

BINARY ?= $(REPO_ROOT)/benchmarks/dhrystone.riscv
CONFIG ?= small

SOURCE_DIR ?= $(mkfile_dir)
BUILD_DIR ?= $(REPO_ROOT)/build/rocket-$(CONFIG)
$(shell mkdir -p $(BUILD_DIR))

SOURCE_MODEL ?= $(SOURCE_DIR)/rocket
BUILD_MODEL ?= $(BUILD_DIR)/rocket

ARCILATOR_ARGS ?= --mlir-timing --print-debug-info
VERILATOR_ARGS ?= -DPRINTF_COND=0 -DASSERT_VERBOSE_COND=0 -DSTOP_COND=0
ESSENT_ARGS ?= -O3

TRACE ?= 0
RUN_ESSENT ?= 0
RUN_ARC ?= 1
RUN_VTOR ?= 1

BENCHMARK_MODE ?= 0
BENCHMARK_OUT_FILE ?= $(BUILD_DIR)/measurements-E$(RUN_ESSENT)A$(RUN_ARC)V$(RUN_VTOR)-$(shell date --iso=seconds).txt
# BENCHMARK_OUT_FILE := ${BENCHMARK_OUT_FILE}
BENCHMARK_OUT_DIR := $(dir $(BENCHMARK_OUT_FILE))
$(shell mkdir -p $(BENCHMARK_OUT_DIR))

BENCHMARK_PREFIX =
BENCHMARK_SUFFIX =
ifeq ($(BENCHMARK_MODE),1)
	BENCHMARK_PREFIX += perf stat -ddd 
	BENCHMARK_SUFFIX += 2>&1 | tee -a $(BENCHMARK_OUT_FILE)
endif

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
	echo "START: Current timestamp in milliseconds: ${shell date +%s%3N}" $(BENCHMARK_SUFFIX)

$(BUILD_MODEL).mlir: $(BUILD_MODEL).fir
	echo "GROUP: arcilator SUBGROUP: firtool COMMAND: firtool --ir-hw $< -o $@" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(FIRTOOL) --ir-hw $< -o $@ $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Arcilator
#===-------------------------------------------------------------------------===

statistics: $(BUILD_MODEL)-statistics.json

$(BUILD_MODEL)-statistics.json: $(BUILD_MODEL).fir $(BUILD_MODEL).mlir
	echo "[" >> $(BUILD_MODEL)-statistics.json
	firtool --parse-only $(BUILD_MODEL).fir | circt-opt --arc-print-statistics -o /dev/null >> $(BUILD_MODEL)-statistics.json
	arcilator --print-arc-statistics $(BUILD_MODEL).mlir -o /dev/null >> $(BUILD_MODEL)-statistics.json
	truncate -s -2 $(BUILD_MODEL)-statistics.json
	echo "]" >> $(BUILD_MODEL)-statistics.json

$(BUILD_MODEL)-arc.o $(BUILD_MODEL).json &: $(BUILD_MODEL).mlir
	echo "GROUP: arcilator SUBGROUP: arcilator COMMAND: arcilator $< --state-file=$(BUILD_MODEL).json -o $(BUILD_MODEL)-arc.ll $(ARCILATOR_ARGS)" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(ARCILATOR) $< --state-file=$(BUILD_MODEL).json -o $(BUILD_MODEL)-arc.ll $(ARCILATOR_ARGS) $(BENCHMARK_SUFFIX)
	echo "GROUP: arcilator SUBGROUP: opt COMMAND: opt $(CXX_OPT_LEVEL) -S $(BUILD_MODEL)-arc.ll -o $(BUILD_MODEL)-llvm.ll" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(OPT) $(CXX_OPT_LEVEL) -S $(BUILD_MODEL)-arc.ll -o $(BUILD_MODEL)-llvm.ll $(BENCHMARK_SUFFIX)
	echo "GROUP: arcilator SUBGROUP: llc COMMAND: llc $(BUILD_MODEL)-llvm.ll -O3 --filetype=obj -o $(BUILD_MODEL)-arc.o" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(LLC) $(BUILD_MODEL)-llvm.ll -O3 --filetype=obj -o $(BUILD_MODEL)-arc.o $(BENCHMARK_SUFFIX)
	objdump -d $(BUILD_MODEL)-arc.o > $(BUILD_MODEL)-arc.s

$(BUILD_MODEL)-arc.h: $(BUILD_MODEL).json
#	echo "GROUP: arcilator SUBGROUP: arcilator COMMAND: python3 $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@" $(BENCHMARK_SUFFIX)
	python3 $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< --view-depth 1 > $@

#===-------------------------------------------------------------------------===
# Verilator
#===-------------------------------------------------------------------------===

$(BUILD_MODEL).sv: $(BUILD_MODEL).fir
	echo "GROUP: verilator SUBGROUP: firtool COMMAND: firtool --lowering-options=disallowLocalVariables $< -o $@" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(FIRTOOL) --lowering-options=disallowLocalVariables $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-vtor.a $(BUILD_MODEL)-vtor.h &: $(BUILD_MODEL).sv $(REPO_ROOT)/verilator-stubs.sv
	echo "GROUP: verilator SUBGROUP: verilator COMMAND: verilator --public -sv -cc -Mdir $(BUILD_MODEL)-vtor $^ --build -j 0 -O3 -Wno-WIDTH -CFLAGS -DVL_TIME_CONTEXT $(VERILATOR_ARGS)" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(VERILATOR) --threads 1 --public -sv -cc -Mdir $(BUILD_MODEL)-vtor $^ --build -j 0 -O3 -Wno-WIDTH -CFLAGS -DVL_TIME_CONTEXT $(VERILATOR_ARGS) $(BENCHMARK_SUFFIX)
	cp $(BUILD_MODEL)-vtor/Vrocket__ALL.a $(BUILD_MODEL)-vtor.a
	cp $(BUILD_MODEL)-vtor/Vrocket.h $(BUILD_MODEL)-vtor.h

#===-------------------------------------------------------------------------===
# Essent
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-essent.h: $(BUILD_MODEL).fir
	cd $(REPO_ROOT)/essent && sbt assembly
	echo "GROUP: essent SUBGROUP: essent COMMAND: $(REPO_ROOT)/essent/utils/bin/essent $(ESSENT_ARGS) $(BUILD_MODEL).fir" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(REPO_ROOT)/essent/utils/bin/essent $(ESSENT_ARGS) $(BUILD_MODEL).fir $(BENCHMARK_SUFFIX)
	mv $(BUILD_DIR)/RocketSystem.h $(BUILD_MODEL)-essent.h

#===-------------------------------------------------------------------------===
# Testbench
#===-------------------------------------------------------------------------===

$(BUILD_MODEL)-model-arc.o: $(SOURCE_MODEL)-model-arc.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-arc.h
	echo "GROUP: arcilator SUBGROUP: clang COMMAND: $(CXX) $(CXXFLAGS) -g -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -c $< -o $@" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -g -I$(ARCILATOR_UTILS_ROOT)/ -I$(BUILD_DIR) -c $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-model-vtor.o: $(SOURCE_MODEL)-model-vtor.cpp $(SOURCE_MODEL)-model.h $(BUILD_MODEL)-vtor.h
	echo "GROUP: verilator SUBGROUP: clang COMMAND: $(CXX) $(CXXFLAGS) -g -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -g -I$(BUILD_DIR) -I/$(VERILATOR_ROOT)/include -c $< -o $@ $(BENCHMARK_SUFFIX)

$(BUILD_MODEL)-model-essent.o: $(SOURCE_MODEL)-model-essent.cpp $(BUILD_MODEL)-essent.h $(SOURCE_MODEL)-model.h
	echo "GROUP: essent SUBGROUP: clang COMMAND: $(CXX) $(CXXFLAGS) -g -I$(BUILD_DIR) -I$(REPO_ROOT)/essent-utils -fbracket-depth=1024 -c $< -o $@" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -g -I$(BUILD_DIR) -I$(REPO_ROOT)/essent-utils -fbracket-depth=1024 -c $< -o $@ $(BENCHMARK_SUFFIX)

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
	echo "GROUP: common SUBGROUP: clang COMMAND: $(CXX) $(CXXFLAGS) -g -latomic -I$(REPO_ROOT)/elfio $^ -o $@" $(BENCHMARK_SUFFIX)
	$(BENCHMARK_PREFIX) $(CXX) $(CXXFLAGS) -g -latomic -I$(REPO_ROOT)/elfio $^ -o $@ $(BENCHMARK_SUFFIX)

#===-------------------------------------------------------------------------===
# Convenience
#===-------------------------------------------------------------------------===

binary-size: $(BUILD_MODEL)-main
	ls -al $(BUILD_DIR) $(BENCHMARK_SUFFIX)

build: $(BUILD_MODEL)-main
	echo "END: Current timestamp in milliseconds: ${shell date +%s%3N}" $(BENCHMARK_SUFFIX)

run: $(BUILD_MODEL)-main
	$(BENCHMARK_PREFIX) $(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS) $(BENCHMARK_SUFFIX)
# sudo chrt -f 99 time --verbose $(BUILD_MODEL)-main $(BINARY) $(RUN_ARGS) >>$(BENCHMARK_OUT_FILE) 2>&1

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
