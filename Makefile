CXXFLAGS = -O3 -Wall -no-pie

mkfile_path := $(dir $(MAKEFILE_LIST))

ARCILATOR_UTILS_ROOT ?= $(dir $(shell which arcilator))
MODEL ?= rocket
BINARY ?= $(mkfile_path)/benchmarks/dhrystone/dhrystone.riscv


$(MODEL).fir: $(mkfile_path)/$(MODEL)/$(MODEL).fir.gz
	gzip -dc $< > $@

$(MODEL).mlir: $(MODEL).fir
	firtool --dedup=1 --ir-hw $< -o $@

$(MODEL).o $(MODEL).json: $(MODEL).mlir
	arcilator $< --state-file=$(MODEL).json | opt -O3 --strip-debug -S | llc -O3 --filetype=obj -o $(MODEL).o

$(MODEL).h: $(MODEL).json
	python $(ARCILATOR_UTILS_ROOT)/arcilator-header-cpp.py $< > $@

$(MODEL)-main: $(mkfile_path)/$(MODEL)/$(MODEL)-main.cpp $(MODEL).h $(MODEL).o
	$(CXX) $(CXXFLAGS) -I$(ARCILATOR_UTILS_ROOT)/ -I$(mkfile_path)/elfio/ -I. $^ -o $@

run: $(MODEL)-main
	./$(MODEL)-main $(BINARY)

clean:
	rm -f $(MODEL).fir $(MODEL).mlir $(MODEL).h $(MODEL).o $(MODEL).json $(MODEL)-main

.PHONY: clean run
