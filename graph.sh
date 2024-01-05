#!/bin/bash

ITERS=5

if [ -z ${2+x} ]; then
	thp_status=$(< /sys/kernel/mm/transparent_hugepage/enabled)
	thp=$(echo "$thp_status" | sed 's/.*\[\(.*\)\].*/\1/')
	balancing=$(< /proc/sys/kernel/numa_balancing)
else
	thp="$2"
	balancing="$3"
fi

prefix="${thp}_${balancing}"
echo "Running for thp=$thp and balancing=$balancing $prefix"

if [ "$1" = "run" ]; then

	for size in 8 16 32; do
		for i in `seq 1 $ITERS`; do 
			echo "Running test $i for size ${size}g"; 
			./run.sh run cmd docker run --rm --volumes-from twitter-data \
				-e WORKLOAD_NAME=pr cloudsuite/graph-analytics \
				--driver-memory "${size}g" \
				--executor-memory "8g" > "/tmp/t/${prefix}_${size}_${i}"
			sleep 5
		done
	done
elif [ "$1" = "process" ]; then
	for size in 8 16 32; do
		echo "---Runtimes for size $size"
		runtimes=()
		ids=()
		for i in `seq 1 $ITERS`; do
			t=`grep "Running time" "/tmp/t/${prefix}_${size}_${i}" | cut -d'=' -f2 | xargs`
			id=`grep cpustat "/tmp/t/${prefix}_${size}_${i}"  | cut -d'_' -f2 | cut -d'.' -f1`
			runtimes+=($t)
			ids+=($id)
		done
		echo "TEST= ${ids[@]}"
		echo "runtime= ${runtimes[@]}"
		python3 -c 'import sys; import numpy as np; arr = np.array(sys.argv[1:], dtype=float); print(f"avg= {np.mean(arr)}\nstd= {np.std(arr)}")' "${runtimes[@]}"
	done
fi

