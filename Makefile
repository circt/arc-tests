CXXFLAGS = -O3 -Wall -no-pie

mkfile_path := $(dir $(MAKEFILE_LIST))

ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
MODEL ?= rocket
BINARY ?= $(mkfile_path)/benchmarks/dhrystone/dhrystone.riscv
TOP_NAME ?= DigitalTop
VERILATOR_ROOT ?= $(shell verilator -getenv VERILATOR_ROOT)

$(MODEL).fir: $(mkfile_path)/$(MODEL)/$(MODEL).fir.gz
	gzip -dc $< > $@

$(MODEL).mlir: $(MODEL).fir
	firtool --dedup=1 --ir-hw $< -o $@

$(MODEL).o $(MODEL).json: $(MODEL).mlir
	arcilator $< --state-file=$(MODEL).json | opt -O3 --strip-debug -S | llc -O3 --filetype=obj -o $(MODEL).o

$(MODEL).h: $(MODEL).json
	python $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< > $@

$(MODEL).sv: $(MODEL).fir
	firtool --verilog --dedup=1 $< -o $@
	cat $(mkfile_path)/EICG_wrapper.v >> $(MODEL).sv
	cat $(mkfile_path)/plusarg_reader.v >> $(MODEL).sv

$(MODEL)-main: $(mkfile_path)/$(MODEL)/$(MODEL)-main.cpp $(MODEL).h $(MODEL).o
	$(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(mkfile_path)/elfio/ -I. $^ -o $@

asm: $(MODEL).mlir
	arcilator $< | opt -O3 --strip-debug -S | llc -O3 --filetype=asm -o $(MODEL).s

run: $(MODEL)-main
	./$(MODEL)-main $(BINARY)

$(MODEL)-vtor/V$(TOP_NAME)__ALL.a: $(MODEL).sv
	verilator -O3 -Mdir $(MODEL)-vtor -sv -cc --trace --build --top-module $(TOP_NAME) $< -j 0

$(MODEL)-verilator-main: $(mkfile_path)/$(MODEL)/$(MODEL)-verilator-main.cpp $(MODEL)-vtor/V$(TOP_NAME)__ALL.a $(VERILATOR_ROOT)/include/verilated.cpp $(VERILATOR_ROOT)/include/verilated_vcd_c.cpp $(VERILATOR_ROOT)/include/verilated_threads.cpp
	g++ $(CXXFLAGS)  -I$(MODEL)-vtor -I$(mkfile_path)/elfio/ -I$(VERILATOR_ROOT)/include/  $^ -o $@

verilate: $(MODEL)-verilator-main
	./$(MODEL)-verilator-main $(BINARY)

clean:
	rm -rf $(MODEL).fir $(MODEL).mlir $(MODEL).h $(MODEL).o $(MODEL).json $(MODEL)-main $(MODEL).sv $(MODEL)-vtor $(MODEL)-verilator-main

.PHONY: clean run
