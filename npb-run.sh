#!/bin/bash

# Author: Karim Manaouil <k.manaouil@gmail.com>
# 
# The University of Edinburgh - 2024
#

CLASSES=("B")
APPS=("is" "ep" "cg" "mg" "ft" "bt" "sp" "lu")

OMP_PATH="/home/karim/omp/"

CG_PATH="/sys/fs/cgroup/npb"
CG_PROCS="$CG_PATH/cgroup.procs"
CG_MEMS="$CG_PATH/cpuset.mems"

function check_cgroup() {
	if ! test -w "$CG_PATH"; then
		echo "cgroup doesn't exist or not root"
		exit 1
	fi
}

function enter_cgroup() {
	check_cgroup
	echo $$ > "$CG_PROCS"
}

function build_app() {
	pushd "$OMP_PATH"
	make "$1" CLASS="$2"
	popd
}

function run() {
	pushd $OMP_PATH 2>&1 >/dev/null
	out=$(./bin/$1 | egrep "(Time in seconds|Mop\/s total)" | cut -d"=" -f2 | xargs)
	secs=$(echo $out | awk '{ print $1 }')
	mops=$(echo $out | awk '{ print $2 }')
	printf "%s,seconds=%s,mops=%s\n" $1 $secs $mops
	popd 2>&1 >/dev/null
}

function run_all() {
	for class in ${CLASSES[@]}; do
		for app in ${APPS[@]}; do
			run "$app.$class.x"
		done
	done
}

while [ "$#" -ne 0 ]; do
	case "$1" in
	"benchmark")
		benchmark=1
		break
	;;
	"compile")
		compile=1
		shift
	;;
	"run")
		run=$2
		shift 2
	;;
	"omp")
		OMP_PATH="$2"
		shift 2
	;;
	*)
		echo "Option $1 not recognised"
		exit 1
	;;
	esac	
done

if test -v "benchmark"; then
	enter_cgroup
	for node in `seq 0 4`; do
		echo "NUMA node $node"
		echo $node > "$CG_MEMS"
		run_all
	done
fi

if test -v "compile"; then
	for class in ${CLASSES[@]}; do
		for app in ${APPS[@]}; do
			build_app "$app" "$class"
		done	
	done
fi

if test -v "run"; then
	enter_cgroup
	if [ "$run" == "all" ]; then
		run_all		
	else
		run "$run"
	fi
fi
