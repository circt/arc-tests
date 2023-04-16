# Arcilator end-to-end tests and benchmarks

This repository collects a set of hardware designs and software to perform
end-to-end tests of the arcilator simulation flow and benchmark its performance.

The `largeBoom` directory contains a compiled and slightly modified version of
the [The Berkeley Out-of-Order RISC-V Processor](https://github.com/riscv-boom/riscv-boom)
published under the licenses in the same folder using
[Chipyard](https://github.com/ucb-bar/chipyard) with the `largeBoomConfig`
configuration. It also contains a custom-written driver for Arcilator.

The `rocket` directory contains a compiled and slightly modified version of
[Rocket Chip](https://github.com/chipsalliance/rocket-chip) published under the
licenses in the same folder using the default configuration in
[Chipyard](https://github.com/ucb-bar/chipyard). It also contains a
custom-written driver for Arcilator.

The `benchmarks` directory contains a set of benchmarks already compiled to ELF
files for easy performance measuring of Arcilator. A benchmark can be run using
the command `make run MODEL=model-name BINARY=binary-path`. Running `make run`
uses `make run MODEL=rocket BINARY=benchmarks/dhrystone/dhrystone.riscv`.
