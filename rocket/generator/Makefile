all: bootrom.img

bootrom.img: bootrom.S
	riscv64-unknown-elf-as bootrom.S -o bootrom.elf
	riscv64-unknown-elf-objcopy -O binary bootrom.elf bootrom.img
	truncate -s 128 bootrom.img
