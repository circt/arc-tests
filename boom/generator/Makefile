GCC=riscv64-unknown-elf-gcc
OBJCOPY=riscv64-unknown-elf-objcopy

all: bootrom.rv64.img

%.elf: %.S
	$(GCC) $< -Tlinker.ld -nostdlib -static -Wl,--no-gc-sections -o $@
%.img: %.bin
	dd if=$< of=$@ bs=128 count=1
%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.bin *.img
