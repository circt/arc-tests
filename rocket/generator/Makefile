ROCKET_URL ?= https://github.com/chipsalliance/rocket-chip.git
ROCKET_TAG ?= v1.6

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

../rocket-%-$(ROCKET_TAG).fir.gz: lib/rocketchip-$(ROCKET_TAG).jar arc.scala bootrom.img
	mkdir -p build
	cd lib && find . -type f -not -name 'rocketchip-$(ROCKET_TAG).jar' -delete
	sbt "runMain freechips.rocketchip.system.Generator -td build -T arc.RocketSystem -C arc.$(CONFIG)"
	gzip -c build/arc.$(CONFIG).fir > $@

lib/rocketchip-$(ROCKET_TAG).jar: rocket-chip-$(ROCKET_TAG)
	mkdir -p lib
	cd rocket-chip-$(ROCKET_TAG) && sbt assembly
	cp rocket-chip-$(ROCKET_TAG)/rocketchip.jar $@

rocket-chip-$(ROCKET_TAG):
	git clone --depth 1 --branch $(ROCKET_TAG) --recursive $(ROCKET_URL) $@

bootrom.img: bootrom.S
	riscv64-unknown-elf-as bootrom.S -o bootrom.elf
	riscv64-unknown-elf-objdump -d bootrom.elf > bootrom.dump
	riscv64-unknown-elf-objcopy -O binary bootrom.elf bootrom.img
	truncate -s 128 bootrom.img

.PHONY: all build-config
