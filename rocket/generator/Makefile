ROCKET_URL ?= https://github.com/chipsalliance/rocket-chip.git
ROCKET_TAG ?= master

all: bootrom.img
all: small dual-small medium dual-medium large dual-large

small: CONFIG=SmallConfig
small: ../rocket-small-$(ROCKET_TAG).fir.gz

dual-small: CONFIG=DualSmallConfig
dual-small: ../rocket-dual-small-$(ROCKET_TAG).fir.gz

medium: CONFIG=MediumConfig
medium: ../rocket-medium-$(ROCKET_TAG).fir.gz

dual-medium: CONFIG=DualMediumConfig
dual-medium: ../rocket-dual-medium-$(ROCKET_TAG).fir.gz

large: CONFIG=LargeConfig
large: ../rocket-large-$(ROCKET_TAG).fir.gz

dual-large: CONFIG=DualLargeConfig
dual-large: ../rocket-dual-large-$(ROCKET_TAG).fir.gz

../rocket-%-$(ROCKET_TAG).fir.gz: lib/rocketchip-$(ROCKET_TAG).jar src/arc.scala bootrom.img mill
	[ ! -d build ] || rm -r build
	mkdir build
	./mill --mill-version 0.11.11 run --top arc.RocketSystem --config arc.$(CONFIG) --dir $(PWD)/build
	# Hacky fix for `error: sink "clint.io_rtcTick" not fully initialized in "RocketSystem"`
	sed 's/inst clint of CLINT.*/&\n    clint.io.rtcTick is invalid/' build/RocketSystem.fir > build/RocketSystem.fixed.fir
	gzip -c build/RocketSystem.fixed.fir > $@

lib/rocketchip-$(ROCKET_TAG).jar: rocket-chip-$(ROCKET_TAG) mill
	[ ! -d lib ] || rm -r lib
	mkdir lib
	cd rocket-chip-$(ROCKET_TAG) && ../mill rocketchip[5.0.0].assembly
	cp rocket-chip-$(ROCKET_TAG)/out/rocketchip/5.0.0/assembly.dest/out.jar $@

rocket-chip-$(ROCKET_TAG):
	git clone --depth 1 --branch $(ROCKET_TAG) --recursive $(ROCKET_URL) $@

bootrom.img: bootrom.S
	riscv64-unknown-elf-as bootrom.S -o bootrom.elf
	riscv64-unknown-elf-objdump -d bootrom.elf > bootrom.dump
	riscv64-unknown-elf-objcopy -O binary bootrom.elf bootrom.img
	truncate -s 128 bootrom.img

mill:
	wget https://raw.githubusercontent.com/lefou/millw/main/millw -O $@
	chmod +x $@

.PHONY: all
