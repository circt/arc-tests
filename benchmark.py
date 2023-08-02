#!/usr/bin/env python3

# Example: ./benchmark.py -- build/rocket-small/rocket-main benchmarks/dhrystone/dhrystone.riscv

from collections import OrderedDict
import argparse
import json
import re
import shlex
import subprocess
import sys

# Parse the command line arguments.
parser = argparse.ArgumentParser(description="Benchmark a hardware simulator")
parser.add_argument("program", metavar="PROGRAM", help="program to benchmark")
parser.add_argument("args",
                    metavar="PROGRAM_ARGS",
                    nargs="*",
                    help="arguments to be passed to the program")
parser.add_argument("-m",
                    "--metadata",
                    metavar=("KEY", "VALUE"),
                    nargs=2,
                    action="append",
                    default=list(),
                    help="additional fields to add to the output JSON")
parser.add_argument("-v",
                    "--verbose",
                    action="store_true",
                    help="show output of hyperfine/callgrind")
args = parser.parse_args()

# Run the benchmark through hyperfine to collect a few useful outputs.
program_cmd = [args.program] + args.args
hfout = subprocess.check_output(
    ["hyperfine", "--warmup=3", "--show-output",
     shlex.join(program_cmd)],
    stderr=subprocess.STDOUT,
    text=True)
if args.verbose:
    sys.stderr.write("----- 8< ----- hyperfine ----- 8< -----\n")
    sys.stderr.write(hfout)

# Compute the execution frequency median.
freqs = list()
for match, _ in re.findall(r'(\d+(\.\d+)?) Hz', hfout):
    freqs.append(float(match))
freqs.sort()
if not freqs:
    print("no frequency info found in simulator output", file=sys.stderr)
    sys.exit(1)
freq = freqs[len(freqs) // 2]

# Run the benchmark through callgrind to collect detailed instruction/data
# access statistics.
cgout = subprocess.check_output(
    [
        "valgrind", "--tool=callgrind", "--simulate-cache=yes",
        "--callgrind-out-file=/dev/null", "--"
    ] + program_cmd,
    stderr=subprocess.STDOUT,
    text=True,
)
if args.verbose:
    sys.stderr.write("----- 8< ----- callgrind ----- 8< -----\n")
    sys.stderr.write(cgout)

# Collect the instruction and data access counts.
events_match = re.search(r'=\s*Events\s*:\s*(.+)', cgout)
collected_match = re.search(r'=\s*Collected\s*:\s*(.+)', cgout)
if not events_match or not collected_match:
    print("no instruction/data access information in callgrind output",
          file=sys.stderr)
    sys.exit(1)
events = dict(
    zip(events_match[1].split(), map(int, collected_match[1].split())))
if "Ir" not in events:
    print("missing instruction accesses in callgrind output", file=sys.stderr)
    sys.exit(1)
if "Dr" not in events:
    print("missing data reads in callgrind output", file=sys.stderr)
    sys.exit(1)
if "Dw" not in events:
    print("missing data writes in callgrind output", file=sys.stderr)
    sys.exit(1)

# Produce the output data.
output = OrderedDict()
output["freq"] = freq
output["exec_inst"] = events["Ir"]
output["data_reads"] = events["Dr"]
output["data_writes"] = events["Dw"]
output["data_accesses"] = events["Dr"] + events["Dw"]
for key, value in args.metadata:
    output[key] = value
print(json.dumps(output))
