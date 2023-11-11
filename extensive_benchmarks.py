#!/usr/bin/env python3

from enum import Enum
import string
import os.system;
import re;

class Simulator(Enum):
    Arcilator = 0
    Verilator = 1
    Essent = 2

    def to_string(self):
        if self == Simulator.Arcilator:
            return "arcilator"
        elif self == Simulator.Verilator:
            return "verilator"
        elif self == Simulator.Essent:
            return "essent"

class Design(Enum):
    BoomSmall = 0
    BoomMedium = 1
    BoomLarge = 2
    BoomMega = 3
    RocketSmall14 = 4
    RocketMedium14 = 5
    RocketLarge14 = 6
    RocketDualSmall14 = 7
    RocketDualMedium14 = 8
    RocketDualLarge14 = 9
    RocketSmall16 = 10
    RocketMedium16 = 11
    RocketLarge16 = 12
    RocketDualSmall16 = 13
    RocketDualMedium16 = 14
    RocketDualLarge16 = 15

    def to_string(self):
        if self == Design.BoomSmall:
            return "small"
        elif self == Design.BoomMedium:
            return "medium"
        elif self == Design.BoomLarge:
            return "large"
        elif self == Design.BoomMega:
            return "mega"
        elif self == Design.RocketSmall14:
            return "small-v1.4"
        elif self == Design.RocketMedium14:
            return "medium-v1.4"
        elif self == Design.RocketLarge14:
            return "large-v1.4"
        elif self == Design.RocketDualSmall14:
            return "dual-small-v1.4"
        elif self == Design.RocketDualMedium14:
            return "dual-medium-v1.4"
        elif self == Design.RocketDualLarge14:
            return "dual-large-v1.4"
        elif self == Design.RocketSmall16:
            return "small-v1.6"
        elif self == Design.RocketMedium16:
            return "medium-v1.6"
        elif self == Design.RocketLarge16:
            return "large-v1.6"
        elif self == Design.RocketDualSmall16:
            return "dual-small-v1.6"
        elif self == Design.RocketDualMedium16:
            return "dual-medium-v1.6"
        elif self == Design.RocketDualLarge16:
            return "dual-large-v1.6"

    def is_rocket_v14(self) -> bool:
        return self >= 4 and self < 10

    def is_rocket_v16(self) -> bool:
        return self >= 10

    def is_rocket(self) -> bool:
        return self.is_rocket_v14() or self.is_rocket_v16()

    def is_boom(self) -> bool:
        return not self.is_rocket()

    def is_dual_core(self) -> bool:
        return (self.is_rocket_v14() and self >= 7) or (self.is_rocket_v16() and self >= 13)

    def is_single_core(self) -> bool:
        return not self.is_single_core()
    
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
    run_essent = "1" if simulator == Simulator.Essent else "0"
    run_arcilator = "1" if simulator == Simulator.Arcilator else "0"
    run_verilator = "1" if simulator == Simulator.Verilator else "0"

    # Build the benchmark
    os.system(f'time make -C {rocket_or_boom} {action} CXX_OPT_LEVEL={cxx_opt_level} BUILD_DIR={build_dir} CONFIG={design.to_string()} ARCILATOR_ARGS="{config.options}" ESSENT_ARGS="{config.options}" RUN_ESSENT={run_essent} RUN_ARC={run_arcilator} RUN_VTOR={run_verilator}')

    # Rocket v1.6:
    # [done] Arcilator O3 with and without deduplication, both sim perf and compile time
    # [done] Arcilator O3 with and without LUTs, both sim perf and compile time
    # [done] Arcilator O0 and dedup disabled, sim perf
    # [done] Arcilator O1 (only canonicalizer and CSE) and dedup disabled, sim perf
    # [done] Verilator
    # [done] Arcilator O3, Verilator binary size
    # [done] Arcilator O3, Verilator compile time

    # Boom:
    # [done] Verilator, clang O3
    # [done] Arcilator O3, clang O3
    # [done] Arcilator O3, Verilator binary size
    # [done] Arcilator O3, Verilator compile time

    # Rocket v1.4:
    # [done] Essent O0, O2, O3, clang O3
    # [done] Verilator, clang O3
    # [done] Arcilator O3, clang O3
    # [done] Arcilator O3, Verilator, Essent O3 with clang -Os
    # [done] Arcilator O3, Essent O3, Verilator binary size
    # [done] Arcilator O3, Verilator, Essent O3 compile time

boom_designs = [
    Design.BoomSmall,
    Design.BoomMedium,
    Design.BoomLarge,
    Design.BoomMega,
]

rocket14_designs = [
    Design.RocketSmall14,
    Design.RocketMedium14,
    Design.RocketLarge14,
    Design.RocketDualSmall14,
    Design.RocketDualMedium14,
    Design.RocketDualLarge14,
]

rocket16_designs = [
    Design.RocketSmall16,
    Design.RocketMedium16,
    Design.RocketLarge16,
    Design.RocketDualSmall16,
    Design.RocketDualMedium16,
    Design.RocketDualLarge16,
]

all_designs = boom_designs ++ rocket14_designs ++ rocket16_designs

def collect_binary_size():
    for design in all_designs:
        config = Config(Simulator.Arcilator, design, "--mlir-timing -O3")
        os.system(f'mkdir -p ./measurements/{config.sim.to_string()}-{design.to_string()}')
        os.system(f'mkdir -p ./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')
        invoke_make(config, "binary-size", f'./measurements/{config.sim.to_string()}-{design.to_string()}/binary-size.txt', f'./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')
        config = Config(Simulator.Verilator, design, "-O3")
        os.system(f'mkdir -p ./measurements/{config.sim.to_string()}-{design.to_string()}')
        os.system(f'mkdir -p ./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')
        invoke_make(config, "binary-size", f'./measurements/{config.sim.to_string()}-{design.to_string()}/binary-size.txt', f'./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')

    for design in rocket14_designs:
        config = Config(Simulator.Essent, design, "-O3")
        os.system(f'mkdir -p ./measurements/{config.sim.to_string()}-{design.to_string()}')
        os.system(f'mkdir -p ./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')
        invoke_make(config, "binary-size", f'./measurements/{config.sim.to_string()}-{design.to_string()}/binary-size.txt', f'./build/{config.sim.to_string()}-{design.to_string()}-binary-size/')


# def extract_measurements(output_file_name):
#     re.search(r'START:')
#     re.search(r'GROUP: arcilator SUBGROUP: firtool')
#     re.search(r'seconds time elapsed')
#     re.search(r'END:')

def benchmark_compile_time(config: Config, uniquifier: string):
    os.system(f'mkdir -p ./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}')

    # Warmup
    for i in range(3):
        measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/compile-time-warmup-{i}.txt'
        build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-compile-time-warmup-{i}/'
        os.system(f'mkdir -p {build_dir}')
        invoke_make(config, "build", measurement_file, build_dir)

    # Runs
    for i in range(10):
        measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/compile-time-run-{i}.txt'
        build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-compile-time-run-{i}/'
        os.system(f'mkdir -p {build_dir}')
        invoke_make(config, "build", measurement_file, build_dir)

def benchmark_compile_time_all():
    for design in rocket16_designs:
        benchmark_compile_time(Config(Simulator.Arcilator, design, "-O3 --dedup=0"), "O3-nodedup")
        benchmark_compile_time(Config(Simulator.Arcilator, design, "-O3 --lookup-tables=0"), "O3-nolut")

    for design in all_designs:
        benchmark_compile_time(Config(Simulator.Arcilator, design, "-O3"), "O3")
        benchmark_compile_time(Config(Simulator.Verilator, design, "-O3"), "O3")

    for design in rocket14_designs:
        benchmark_compile_time(Config(Simulator.Essent, design, "-O0"), "O0")
        benchmark_compile_time(Config(Simulator.Essent, design, "-O2"), "O2")
        benchmark_compile_time(Config(Simulator.Essent, design, "-O3"), "O3")
        benchmark_compile_time(Config(Simulator.Essent, design, "-O3"), "Os", "-Os")
        benchmark_compile_time(Config(Simulator.Arcilator, design, "-O3"), "Os", "-Os")
        benchmark_compile_time(Config(Simulator.Verilator, design, "-O3"), "Os", "-Os")

def benchmark_simulation_performance(config: Config, uniquifier: string, cxx_opt_level = "-O3"):
    measurement_file = f'./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}/sim-perf.txt'
    build_dir = f'./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-simperf/'
    os.system(f'mkdir -p ./measurements/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}')
    os.system(f'mkdir -p ./build/{config.sim.to_string()}-{config.design.to_string()}-{uniquifier}-simperf/')
    invoke_make(config, "build", measurement_file, build_dir, cxx_opt_level)
    rocket_or_boom = "rocket" if config.design.is_rocket() else "boom"

    # Warmup
    for i in range(3):
        os.system(f'sudo chrt -f 99 perf stat -ddd {build_dir}{rocket_or_boom}-main benchmarks/dhrystone.riscv >>{measurement_file} 2>&1')

    # Runs
    for i in range(10):
        os.system(f'sudo chrt -f 99 perf stat -ddd {build_dir}{rocket_or_boom}-main benchmarks/dhrystone.riscv >>{measurement_file} 2>&1')

def benchmark_simulation_performance_all():
    for design in rocket16_designs:
        benchmark_simulation_performance(Config(Simulator.Arcilator, design, "-O3 --dedup=0"), "O3-nodedup")
        benchmark_simulation_performance(Config(Simulator.Arcilator, design, "-O0 --dedup=0"), "O0-nodedup")
        benchmark_simulation_performance(Config(Simulator.Arcilator, design, "-O1 --dedup=0"), "O1-nodedup")
        benchmark_simulation_performance(Config(Simulator.Arcilator, design, "-O3 --lookup-tables=0"), "O3-nolut")

    for design in all_designs:
        benchmark_simulation_performance(Config(Simulator.Arcilator, design, "-O3"), "O3")
        benchmark_simulation_performance(Config(Simulator.Verilator, design, "-O3"), "O3")

    for design in rocket14_designs:
        benchmark_simulation_performance(Config(Simulator.Essent, design, "-O3"), "O3")

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
    print("Measuring compile time...")
    print("//----------------------------------------------//")
    benchmark_compile_time_all()
