default: all

src_dir = .

MARCH ?= rv64gcv
MABI ?= lp64d
RISCV_GCC ?= riscv64-unknown-elf-gcc
RISCV_GCC_OPTS ?= -DPREALLOCATE=1 -mcmodel=medany -static -std=gnu99 -O2 -ffast-math -fno-common -fno-builtin-printf -fno-tree-loop-distribute-patterns -march=$(MARCH) -mabi=$(MABI)
RISCV_LINK_OPTS ?= -static -nostdlib -nostartfiles -lm -lgcc -T $(src_dir)/common/test.ld
RISCV_OBJDUMP ?= riscv64-unknown-elf-objdump --disassemble-all --disassemble-zeroes --section=.tohost --section=.text --section=.text.startup --section=.text.init --section=.data

dhrystone.riscv: $(src_dir)/dhrystone/dhrystone.c $(src_dir)/dhrystone/dhrystone_main.c $(src_dir)/common/syscalls.c $(src_dir)/common/crt.S 
	$(RISCV_GCC) -I$(src_dir)/common -I$(src_dir)/dhrystone $(RISCV_GCC_OPTS) -o $@ $^ $(RISCV_LINK_OPTS)

dhrystone.riscv.dump: dhrystone.riscv
	$(RISCV_OBJDUMP) $< > $@

all: dhrystone.riscv.dump

clean:
	rm -rf $(objs) dhrystone.riscv dhrystone.riscv.dump
