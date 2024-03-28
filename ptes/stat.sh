#!/bin/bash

timestamp="$EPOCHSECONDS"

echo "timestap: $timestamp"

do_iomem () {
	echo 0 > /sys/kernel/tracing/trace
	echo 6 > /sys/kernel/mm/multimem/iomem
}

ctrl_c () {
	do_iomem
	grep page_table_map /sys/kernel/tracing/trace |\
		awk '{ print $6 $7 }' > "data/pte_map_$timestamp"
	exit 0
}


trap ctrl_c INT

while true; do
	do_iomem
	dmesg | tail -1 
	sleep 1
done | tee "data/pte_growth_$timestamp"
