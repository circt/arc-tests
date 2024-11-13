GCC=riscv64-unknown-elf-gcc
OBJCOPY=riscv64-unknown-elf-objcopy

all: bootrom.rv64.img
all: small medium large mega dual-small dual-medium dual-large dual-mega

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

dual-small: CONFIG=DualSmallConfig
dual-small: ../boom-dual-small.fir.gz

dual-medium: CONFIG=DualMediumConfig
dual-medium: ../boom-dual-medium.fir.gz

dual-large: CONFIG=DualLargeConfig
dual-large: ../boom-dual-large.fir.gz

dual-mega: CONFIG=DualMegaConfig
dual-mega: ../boom-dual-mega.fir.gz

../boom-%.fir.gz: src/arc.scala bootrom.rv64.img mill
	[ ! -d build ] || rm -r build
	mkdir build
	./mill run -T arc.BoomSystem -C arc.$(CONFIG) -td $(PWD)/build
	# Remove annotations
	sed -i '/%\[\[/,/\]\]/d;1a\circuit BoomSystem :' build/arc.$(CONFIG).fir
	# Remove printf encoded assertions
	sed -i  -e '/when/{N;/\n.*printf/d;}' build/arc.$(CONFIG).fir
	sed -i 's/inst clint_domain of CLINTClockSinkDomain.*/&\n    invalidate clint_domain.tick/' build/arc.$(CONFIG).fir
	gzip -c build/arc.$(CONFIG).fir > $@

clean:
	rm -rf build/ target/ project/ *.bin *.img *.elf ../boom-small.fir.gz ../boom-medium.fir.gz ../boom-large.fir.gz ../boom-mega.fir.gz

mill:
	wget https://raw.githubusercontent.com/lefou/millw/main/millw -O $@
	chmod +x $@

.PHONY: all build-config
