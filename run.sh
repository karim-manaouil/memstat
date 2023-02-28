#!/bin/bash

if [ "$1" = "clean" ]; then
	rm data/*
elif [ "$1" = "vmstat" ]; then
	python3 process_vmstat.py --run
elif [ "$1" = "numastat" ]; then
	python3 process_numastat.py --run
elif [ "$1" = "plot" ]; then
	if [ "$2" = "vmstat" ]; then
		python3 process_vmstat.py --plot numa_pte_updates \
			numa_hint_faults numa_hint_faults_local \
			numa_pages_migrated 
	elif [ "$2" = "numastat" ]; then
		python3 process_numastat.py --plot $3
	elif [ "$2" = "cpustat" ]; then
		python3 process_cpustat.py --plot	
	elif [ "$2" = "pmbench" ]; then
		python3 pmbench_plot.py pmbench_access_latency
	fi

elif [ "$1" = "ima" ]; then
	python3 process_vmstat.py --run &
	pid1=$!
	python3 process_numastat.py --run &
	pid2=$!
	python3 process_cpustat.py --run &
	pid3=$!
	#docker create --name movielens-data cloudsuite/movielens-dataset
	perf stat docker run --volumes-from movielens-data \
		cloudsuite/in-memory-analytics /data/ml-latest /data/myratings.csv \
		--driver-memory 16g \
		--executor-memory 8g \
		--conf spark.executor.extraJavaOptions="-Xss512m" \
		--conf spark.driver.extraJavaOptions="-Xss512m"
	kill -2 $pid1
	kill -2 $pid2
	kill -2 $pid3
	wait
elif [ "$1" = "pmbench" ]; then
	python3 process_vmstat.py --run &
	pid1=$!
	python3 process_numastat.py --run &
	pid2=$!
	python3 process_cpustat.py --run &
	pid3=$!
	# numactl -m 2 --cpunodebind 0 perf stat ~/pmbench/pmbench -i -j4 -m 8192 -s 8192 -p uniform -r 80 30
	perf stat ~/pmbench/pmbench -i -j 4 -m 8192 -s 8192 -p normal --shape=16384 -r 80 30
	kill -2 $pid1
	kill -2 $pid2
	kill -2 $pid3
	wait
fi
