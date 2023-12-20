#!/bin/bash

if [ "$1" = "clean" ]; then
	rm data/*
elif [ "$1" = "plot" ]; then
	if [ "$2" = "vmstat" ]; then
		if [ "$3" = "numa" ]; then
			python3 process_vmstat.py --plot numa_pte_updates \
				numa_hint_faults numa_hint_faults_local \
				numa_pages_migrated  --stat $4
		elif [ "$3" = "thp" ]; then
			python3 process_vmstat.py --plot thp_fault_alloc \
				thp_fault_fallback thp_collapse_alloc \
				thp_deferred_split_page thp_split_pmd  --stat $4
		fi
	elif [ "$2" = "numastat" ]; then
		python3 process_numastat.py --plot $3 --stat $4
	elif [ "$2" = "cpustat" ]; then
		python3 process_cpustat.py --plot --stat $3
	elif [ "$2" = "pmbench" ]; then
		python3 pmbench_plot.py pmbench_access_latency
	fi

elif [ "$1" = "run" ]; then
	timestamp="$EPOCHSECONDS"
	echo "Timestamp $timestamp"

	python3 process_vmstat.py --stat $timestamp --run &
	pid1=$!
	python3 process_numastat.py --stat $timestamp --run &
	pid2=$!
	python3 process_cpustat.py --stat $timestamp --run &
	pid3=$!
	sleep 3

	if [ "$2" = "ima" ]; then
		#docker create --name movielens-data cloudsuite/movielens-dataset
		perf stat docker run --volumes-from movielens-data \
			cloudsuite/in-memory-analytics /data/ml-latest /data/myratings.csv \
			--driver-memory 16g \
			--executor-memory 8g \
			--conf spark.executor.extraJavaOptions="-Xss512m" \
			--conf spark.driver.extraJavaOptions="-Xss512m"

	elif [ "$2" = "pmbench" ]; then
		# numactl -m 2 --cpunodebind 0 perf stat ~/pmbench/pmbench -i -j4 -m 8192 -s 8192 -p uniform -r 80 30
		perf stat ~/pmbench/pmbench -i -j 4 -m 8192 -s 8192 -p normal --shape=16384 -r 80 30
	elif [ "$2" = "bigann" ]; then
		pushd ~/big-ann-benchmarks
		source venv/bin/activate
		python run.py --nodocker --dataset bigann-1B --algorithm faiss-t1
		deactivate
		popd
	elif [ "$2" = "cmd" ]; then
		${@:3}
	fi
		# parsecmgmt -a run -p parsec.bodytrack -i simlarge -n 16 -s "sudo perf stat -M cpu_utilization -a -- "

	sleep 3
	kill -INT $pid1
	kill -INT $pid2
	kill -INT $pid3

	wait $pid1
	wait $pid2
	wait $pid3
fi
