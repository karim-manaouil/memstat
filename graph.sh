#!/bin/bash

if [ "$1" = "run" ]; then
	for size in 8 16; do
		for i in `seq 1 10`; do 
			echo "Running test $i for size ${size}g"; 
			./run.sh run cmd docker run --rm --volumes-from twitter-data \
				-e WORKLOAD_NAME=pr cloudsuite/graph-analytics \
				--driver-memory "${size}g" \
				--executor-memory "8g" > "/tmp/t/${size}_${i}"
			sleep 5
		done
	done
elif [ "$1" = "process" ]; then
	for size in 8 16; do
		echo "---Runtimes for size $size"
		runtimes=()
		for i in `seq 1 10`; do
			t=`grep -e "Running time" "/tmp/t/${size}_${i}" | cut -d'=' -f2 | xargs`
			runtimes+=($t)
		done
		echo "runtimes= ${runtimes[@]}"
		python3 -c 'import sys; import numpy as np; arr = np.array(sys.argv[1:], dtype=float); print(f"avg= {np.mean(arr)}\nstd= {np.std(arr)}")' "${runtimes[@]}"
	done
fi

