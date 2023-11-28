GCC=riscv64-unknown-elf-gcc
OBJCOPY=riscv64-unknown-elf-objcopy

all: bootrom.rv64.img
all: small medium large mega

%.elf: %.S
	$(GCC) $< -Tlinker.ld -nostdlib -static -Wl,--no-gc-sections -o $@
%.img: %.bin
	dd if=$< of=$@ bs=128 count=1
%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

small: CONFIG=SmallConfig
small: ../boom-small.fir.gz

medium: CONFIG=MediumConfig
medium: ../boom-medium.fir.gz

large: CONFIG=LargeConfig
large: ../boom-large.fir.gz

mega: CONFIG=MegaConfig
mega: ../boom-mega.fir.gz

giga: CONFIG=GigaConfig
giga: ../boom-giga.fir.gz

../boom-%.fir.gz: arc.scala bootrom.rv64.img
	mkdir -p build
	sbt "runMain freechips.rocketchip.system.Generator -td build -T arc.BoomSystem -C arc.$(CONFIG)"
	gzip -c build/arc.$(CONFIG).fir > $@

clean:
	rm -rf build/ target/ project/ *.bin *.img *.elf ../boom-small.fir.gz ../boom-medium.fir.gz ../boom-large.fir.gz ../boom-mega.fir.gz

.PHONY: all build-config
