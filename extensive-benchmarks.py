#!/usr/bin/env python3

from enum import IntEnum
import string
import subprocess

class Simulator(IntEnum):
    Arcilator = 0
    Verilator = 1
    VerilatorCIRCT = 2

    def to_string(self):
        if self == Simulator.Arcilator:
            return "arcilator"
        elif self == Simulator.Verilator:
            return "verilator"
        elif self == Simulator.VerilatorCIRCT:
            return "verilator-circt"

class Design(IntEnum):
    BoomSmall = 0
    BoomLarge = 1
    BoomMega = 2
    DualBoomSmall = 3
    DualBoomLarge = 4
    DualBoomMega = 5
    RocketSmall = 6
    RocketMedium = 7
    RocketLarge = 8
    RocketDualSmall = 9
    RocketDualMedium = 10
    RocketDualLarge = 11
    Snitch = 12

    def to_string(self):
        if self == Design.BoomSmall:
            return "small"
        elif self == Design.BoomLarge:
            return "large"
        elif self == Design.BoomMega:
            return "mega"
        if self == Design.DualBoomSmall:
            return "dual-small"
        elif self == Design.DualBoomLarge:
            return "dual-large"
        elif self == Design.DualBoomMega:
            return "dual-mega"
        elif self == Design.RocketSmall:
            return "small-master"
        elif self == Design.RocketMedium:
            return "medium-master"
        elif self == Design.RocketLarge:
            return "large-master"
        elif self == Design.RocketDualSmall:
            return "dual-small-master"
        elif self == Design.RocketDualMedium:
            return "dual-medium-master"
        elif self == Design.RocketDualLarge:
            return "dual-large-master"
        elif self == Design.Snitch:
            return "snitch"

    def is_rocket(self) -> bool:
        return int(self) >= 6

    def is_boom(self) -> bool:
        return not self.is_rocket()
    
    def is_snitch(self) -> bool:
        return int(self) == 12

    def is_dual_core(self) -> bool:
        return int(self) in [3,4,5,9,10,11]

    def is_single_core(self) -> bool:
        return not self.is_dual_core()
    
class Config:
    sim: Simulator
    design: Design
    options: string

    def __init__(self, sim: Simulator, design: Design, options: string):
        self.sim = sim
        self.design = design
        self.options = options


def invoke_make(config: Config, action: string, measurement_file: string, build_dir: string, cxx_opt_level = "-O3"):
    design = config.design
    simulator = config.sim
    rocket_or_boom = "rocket" if design.is_rocket() else "boom"
    makefile_dir = "snitch" if design.is_snitch() else rocket_or_boom
    run_arcilator = "1" if simulator == Simulator.Arcilator else "0"
    run_verilator = "1" if simulator == Simulator.Verilator else "0"
    run_verilator_circt = "1" if simulator == Simulator.VerilatorCIRCT else "0"

    # Build the benchmark
    subprocess.call(f'make -C {makefile_dir} {action} BENCHMARK_OUT_FILE={measurement_file} BENCHMARK_MODE=1 CXX_OPT_LEVEL={cxx_opt_level} BUILD_DIR={build_dir} CONFIG={design.to_string()} RUN_VTOR_CIRCT={run_verilator_circt} RUN_ARC={run_arcilator} RUN_VTOR={run_verilator}', shell=True)

boom_designs = [
    Design.BoomMega,
    Design.BoomLarge,
    Design.BoomSmall,
    Design.DualBoomSmall,
    Design.DualBoomLarge,
    Design.DualBoomMega,
]

rocket_designs = [
    Design.RocketSmall,
    Design.RocketMedium,
    Design.RocketLarge,
    Design.RocketDualSmall,
    Design.RocketDualMedium,
    Design.RocketDualLarge,
]

all_designs = boom_designs + rocket_designs + [Design.Snitch]

def collect_binary_size():
    for design in [Design.Snitch]:
        config = Config(Simulator.Arcilator, design, "--mlir-timing -O3")
        invoke_make(config, "binary-size", f'./measurements/{config.sim.to_string()}-{design.to_string()}/binary-size.txt', f'./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')
        config = Config(Simulator.Verilator, design, "-O3")
        invoke_make(config, "binary-size", f'./measurements/{config.sim.to_string()}-{design.to_string()}/binary-size.txt', f'./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')
        config = Config(Simulator.VerilatorCIRCT, design, "-O3")
        invoke_make(config, "binary-size", f'./measurements/{config.sim.to_string()}-{design.to_string()}/binary-size.txt', f'./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')

def benchmark_compile_time(config: Config, uniquifier: string):
    # Warmup
    for i in range(1):
        measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/compile-time-warmup-{i}.txt'
        build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-compile-time-warmup-{i}/'
        invoke_make(config, "build", measurement_file, build_dir)

    # Run
    for i in range(5):
        measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/compile-time-run-{i}.txt'
        build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-compile-time-run-{i}/'
        invoke_make(config, "build", measurement_file, build_dir)

def benchmark_compile_time_all():
    for design in [Design.Snitch]:
        benchmark_compile_time(Config(Simulator.Arcilator, design, "-O3"), "O3")
        benchmark_compile_time(Config(Simulator.Verilator, design, "-O3"), "O3")
        benchmark_compile_time(Config(Simulator.VerilatorCIRCT, design, "-O3"), "O3")

def benchmark_simulation_performance(config: Config, uniquifier: string, cxx_opt_level = "-O3"):
    measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/sim-perf.txt'
    build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-simperf/'
    invoke_make(config, "build", measurement_file, build_dir, cxx_opt_level)
    rocket_or_boom = "rocket" if config.design.is_rocket() else "boom"

    binary = "benchmarks/dhrystone_rv32i.riscv" if config.design.is_snitch() else "benchmarks/dhrystone_rv64gcv.riscv"

    # Warmup
    for i in range(3):
        subprocess.call(f'{rocket_or_boom}/{build_dir}{rocket_or_boom}-main {binary} 2>&1 | tee -a {rocket_or_boom}/{measurement_file}', shell=True)

    # Runs
    for i in range(10):
        subprocess.call(f'{rocket_or_boom}/{build_dir}{rocket_or_boom}-main {binary} 2>&1 | tee -a {rocket_or_boom}/{measurement_file}', shell=True)

def collect_hardware_counter_info(config: Config, uniquifier: string, cxx_opt_level = "-O3"):
    measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/hardware-counters.txt'
    build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-hardware-counters/'
    invoke_make(config, "build", measurement_file, build_dir, cxx_opt_level)
    rocket_or_boom = "rocket" if config.design.is_rocket() else "boom"

    binary = "benchmarks/dhrystone_rv32i.riscv" if config.design.is_snitch() else "benchmarks/dhrystone_rv64gcv.riscv"

    # Warmup
    subprocess.call(f'perf stat -ddd {rocket_or_boom}/{build_dir}{rocket_or_boom}-main {binary} 2>&1 | tee -a {rocket_or_boom}/{measurement_file}', shell=True)

    # Runs
    for i in range(10):
      subprocess.call(f'perf stat -ddd {rocket_or_boom}/{build_dir}{rocket_or_boom}-main {binary} 2>&1 | tee -a {rocket_or_boom}/{measurement_file}', shell=True)

def benchmark_simulation_performance_all():
    for design in [Design.Snitch]:
        benchmark_simulation_performance(Config(Simulator.Arcilator, design, "-O3"), "O3")
        benchmark_simulation_performance(Config(Simulator.Verilator, design, "-O3"), "O3")
        benchmark_simulation_performance(Config(Simulator.VerilatorCIRCT, design, "-O3"), "O3")

def collect_hardware_counter_info_all():
    for design in [Design.Snitch]:
        collect_hardware_counter_info(Config(Simulator.Arcilator, design, "-O3"), "O3")
        collect_hardware_counter_info(Config(Simulator.Verilator, design, "-O3"), "O3")
        collect_hardware_counter_info(Config(Simulator.VerilatorCIRCT, design, "-O3"), "O3")

if __name__ == "__main__":
    print("//----------------------------------------------//")
    print("Collecting binary sizes...")
    print("//----------------------------------------------//")
    collect_binary_size()
    print("//----------------------------------------------//")
    print("Measuring simulation performance...")
    print("//----------------------------------------------//")
    benchmark_simulation_performance_all()
    print("//----------------------------------------------//")
    print("Collecting Hardware counter metrics...")
    print("//----------------------------------------------//")
    collect_hardware_counter_info_all()
    print("//----------------------------------------------//")
    print("Measuring compile time...")
    print("//----------------------------------------------//")
    benchmark_compile_time_all()
