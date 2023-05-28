#!/usr/bin/env python3
import argparse
import re
import sys
from vcdvcd import VCDVCD, binary_string_to_hex
from typing import *

parser = argparse.ArgumentParser(
    description="Print the first difference between two VCD files")
parser.add_argument("file1", metavar="VCD1", help="first file to compare")
parser.add_argument("file2", metavar="VCD2", help="second file to compare")
parser.add_argument("--top1",
                    metavar="INSTPATH",
                    help="instance in first file to compare")
parser.add_argument("--top2",
                    metavar="INSTPATH",
                    help="instance in second file to compare")
parser.add_argument("-f",
                    "--filter",
                    metavar="REGEX",
                    action="append",
                    default=[],
                    help="only compare signals matching a regex")
parser.add_argument("-i",
                    "--ignore",
                    metavar="REGEX",
                    action="append",
                    default=[],
                    help="ignore signals matching a regex")
parser.add_argument("-l",
                    "--list",
                    action="store_true",
                    help="list signals and exit")
parser.add_argument("-v",
                    "--verbose",
                    action="store_true",
                    help="verbose output")
parser.add_argument("-a", "--after", type=int, help="only compare after time")
parser.add_argument("-b",
                    "--before",
                    type=int,
                    help="only compare before time")
args = parser.parse_args()


def info(s: str):
    if args.verbose:
        sys.stderr.write(s)


def infoln(s: str):
    info(f"{s}\n")


# Collect the signals in both files.
vcd1 = VCDVCD(args.file1, only_sigs=True)
vcd2 = VCDVCD(args.file2, only_sigs=True)
infoln(f"{len(vcd1.signals)} signals in first file")
infoln(f"{len(vcd2.signals)} signals in second file")


# Extract signals under the requested top-level instance.
def filter_signals(signals: List[str],
                   prefix: Optional[str]) -> Dict[str, str]:
    if prefix is None:
        return dict((s, s) for s in signals)
    else:
        return dict(
            (s[len(prefix):], s) for s in signals if s.startswith(prefix))


filtered_signals1 = filter_signals(vcd1.signals, args.top1)
filtered_signals2 = filter_signals(vcd2.signals, args.top2)
if args.top1 is not None:
    infoln(
        f"{len(filtered_signals1)} signals under `{args.top1}` in first file")
if args.top2 is not None:
    infoln(
        f"{len(filtered_signals2)} signals under `{args.top2}` in second file")

# Find the common signals.
common_signals = list()
for key, sig1 in filtered_signals1.items():
    if sig2 := filtered_signals2.get(key):
        common_signals.append((key, sig1, sig2))
common_signals.sort()
infoln(f"{len(common_signals)} common signals")

# Filter the common signals.
for f in args.filter:
    f = re.compile(f)
    common_signals = [x for x in common_signals if f.search(x[0])]

# Filter out ignored signals.
for i in args.ignore:
    i = re.compile(i)
    common_signals = [x for x in common_signals if not i.search(x[0])]

if args.filter or args.ignore:
    infoln(f"{len(common_signals)} filtered and unignored signals")

# List the signals and exit if requested.
if args.list:
    for s in common_signals:
        print(s[0])
    sys.exit(0)

# Abort if there are no common signals to compare.
if not common_signals:
    sys.stderr.write("no commong signals between input files\n")
    sys.exit(1)

# Read the VCD files with only the interesting signals.
infoln("Reading first file")
vcd1 = VCDVCD(args.file1, signals=[x[1] for x in common_signals])
infoln("Reading second file")
vcd2 = VCDVCD(args.file2, signals=[x[2] for x in common_signals])

# Compare each signal.
earliest_mismatches = []
for signal, signame1, signame2 in common_signals:
    infoln(f"Comparing {signal}")
    signal1 = vcd1[signame1]
    signal2 = vcd2[signame2]

    def skip_time(tv: List) -> List:
        if args.after:
            while tv and tv[0][0] < args.after:
                tv = tv[1:]
        return tv

    tv1 = skip_time(signal1.tv)
    tv2 = skip_time(signal2.tv)
    for (t1, v1), (t2, v2) in zip(tv1, tv2):
        if t1 == t2 and v1 == v2:
            continue
        t = min(t1, t2)
        if args.before and t > args.before:
            break
        v1 = signal1[t]
        v2 = signal2[t]
        if v1 == v2:
            continue
        if earliest_mismatches and t < earliest_mismatches[0][0]:
            earliest_mismatches = []
        if not earliest_mismatches or t == earliest_mismatches[0][0]:
            earliest_mismatches.append((t, v1, v2, signal))
        break

for t, sig1, sig2, name in earliest_mismatches:
    print(
        f"{t}  {binary_string_to_hex(sig1)}  {binary_string_to_hex(sig2)}  {name}"
    )
if earliest_mismatches:
    sys.exit(1)
