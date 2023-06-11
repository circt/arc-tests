# Arcilator end-to-end tests and benchmarks

This repository collects a set of hardware designs and software to perform end-to-end tests of the arcilator simulation flow and benchmark its performance.


## Designs

### Rocket

The `rocket` directory contains a compiled and slightly modified version of [Rocket Chip](https://github.com/chipsalliance/rocket-chip) published under the licenses in the same folder using the default configuration in [Chipyard](https://github.com/ucb-bar/chipyard). It also contains a custom-written driver for Arcilator.

Run Rocket benchmarks as follows:

- `make -C rocket run`: Lockstep Arcilator and Verilator simulation. Aborts as soon as the simulations diverge.
- `make -C rocket run-trace`: Lockstep simulation with `rocket-{arcs,vtor}.vcd` output files.
- `make -C rocket run-arcs`: Arcilator only.
- `make -C rocket run-vtor`: Verilator only.

Pass `BINARY=<binary>` to make to run a specific benchmark. Pick one of the configs as follows:

- `CONFIG=small`
- `CONFIG=medium`
- `CONFIG=large`

To generate new Rocket designs, tweak the `rocket/generator/arc.scala` file and run `make -C rocket/generator` to rebuild the `rocket/*.fir.gz` files used for the benchmarks.


### BOOM

The `largeBoom` directory contains a compiled and slightly modified version of the [The Berkeley Out-of-Order RISC-V Processor](https://github.com/riscv-boom/riscv-boom) published under the licenses in the same folder using [Chipyard](https://github.com/ucb-bar/chipyard) with the `largeBoomConfig` configuration. It also contains a custom-written driver for Arcilator.

Run benchmarks with `make run MODEL=largeBoom BINARY=<binary>`.

### Riscinator

Run benchmarks with `make run MODEL=riscinator BINARY=<binary>`.


## Benchmarks

The `benchmarks` directory contains a set of benchmarks already compiled to ELF files for easy performance measuring of Arcilator. The easiest way to run a benchmark is by setting `BINARY=<binary>` when calling `make run` for one of the designs. You can also pass the binary directly to one of the compile simulators.

Available benchmarks:

- `benchmarks/dhrystone/dhrystone.riscv`


## Debugging

To debug discrepancies between the simulators, use the `diffvcd.py` script. For example:

    ./diffvcd.py rocket-{vtor,arcs}.vcd --top1 TOP.RocketSystem. --top2 RocketSystem.internal. -i icache.readEnable -i icache.writeEnable

This command compares the `TOP.RocketSystem` subhierarchy in the first VCD file (Verilator) against the `RocketSystem.internal` subhierarchy in the second VCD file (Arcilator). This allows you to point to the same module in the design even if the two simulators have slightly different ways of wrapping them up at the top-level. The command also ignores any discrepancies in the `icache.readEnable` and `icache.writeEnable` signals.
