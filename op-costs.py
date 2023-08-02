#!/usr/bin/env python3

# Example:
#   callgrind_annotate --show=Ir callgrind.out > callgrind.log
#   cat callgrind.log | ./op-costs.py | sort -n

import re
import sys
from collections import OrderedDict

r1 = re.compile(r'^\s*([0-9,]+)\s+(\(.+?\))?\s+(.*)')
r2 = re.compile(r'(%.+?\b(\s*,\s*%.+?\b)*\s*=\s*)?(\w+\.[\w._]+)\s*(.*)')

counts = OrderedDict()
total_indicated = None

for line in sys.stdin:
    m1 = r1.match(line)
    if not m1:
        continue
    count = int(m1[1].replace(",", ""))
    if m1[3] == "events annotated":
        total_indicated = count
    m2 = r2.match(m1[3])
    if not m2:
        continue
    mnemonic = m2[3]
    counts[mnemonic] = count + counts.get(mnemonic, 0)

total_counted = sum(counts.values())
if total_indicated is None:
    total_indicated = total_counted
counts["(total)"] = total_indicated
if total_counted < total_indicated:
    counts["(other)"] = total_indicated - total_counted

countlen = len(f"{max(counts.values())}")
for mnemonic, count in counts.items():
    print(f"{count:{countlen}}  {count/total_indicated*100:5.1f}%  {mnemonic}")
