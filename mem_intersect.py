# Karim Manaouil, k.manaouil@gmail.com 
# University of Edinburgh 2023
#
# To get the trace to be passed as the argument
# run 'perf record --sample-cpu -d  -a -- command args'
# then
# 'perf report -D -i perf.data | grep RECORD_SAMPLE > trace'  
#

import sys
from collections import defaultdict

def round_to_4kb(address):
    return address & ~(0xFFF)

if len(sys.argv) < 2:
    print("Usage: python script_name.py <filename>")
    sys.exit(1)

file_path = sys.argv[1]
cpu_pages = defaultdict(set)
page_access_count = defaultdict(lambda: defaultdict(int))

with open(file_path, 'r') as file:
    for line in file:
        parts = line.split()
        if len(parts) >= 2 and "0/0" not in line and not line.endswith(" 0\n"):
            cpu = parts[0]
            addr_str = parts[-1]
            try:
                addr = int(addr_str, 16)
                page = round_to_4kb(addr)
                cpu_pages[cpu].add(page)
                page_access_count[page][cpu] += 1
            except ValueError:
                pass

shared_pages = {page: counts for page, counts in page_access_count.items() if len(counts) > 1}

for page, counts in shared_pages.items():
    total_accesses = sum(counts.values())
    num_cpus = len(counts)
    average_accesses = total_accesses / num_cpus
    if average_accesses >= 100:
        print(f"Page {hex(page)} accessed by multiple CPUs:")
        for cpu, access_count in counts.items():
            print(f"  CPU {cpu} accessed {access_count} times")
