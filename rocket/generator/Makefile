ROCKET_URL ?= https://github.com/chipsalliance/rocket-chip.git
ROCKET_TAG ?= v1.6

all: bootrom.img
all: small medium large

small: CONFIG=SmallConfig
small: ../rocket-small.fir.gz

medium: CONFIG=MediumConfig
medium: ../rocket-medium.fir.gz

large: CONFIG=LargeConfig
large: ../rocket-large.fir.gz

../rocket-%.fir.gz: lib/rocketchip.jar arc.scala bootrom.img
	mkdir -p build
	sbt "runMain freechips.rocketchip.system.Generator -td build -T arc.RocketSystem -C arc.$(CONFIG)"
	gzip -c build/arc.$(CONFIG).fir > $@

lib/rocketchip.jar: rocket-chip
	mkdir -p lib
	cd rocket-chip && sbt assembly
	cp rocket-chip/rocketchip.jar $@

rocket-chip:
	git clone --depth 1 --branch $(ROCKET_TAG) --recursive $(ROCKET_URL) $@

bootrom.img: bootrom.S
	riscv64-unknown-elf-as bootrom.S -o bootrom.elf
	riscv64-unknown-elf-objdump -d bootrom.elf > bootrom.dump
	riscv64-unknown-elf-objcopy -O binary bootrom.elf bootrom.img
	truncate -s 128 bootrom.img

.PHONY: all build-config
