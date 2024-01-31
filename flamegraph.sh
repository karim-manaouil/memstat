#!/bin/bash
#
# Karim Manaouil <k.manaouil@gmail.com>
# The University of Edinburgh - 2023-2024
#
# This depends o the FlameGraph tool from
# Brendan Gregg, here
# https://www.brendangregg.com/flamegraphs.html
#
# $1: pid of process to record
# $2: time in sec for recording
#

flmg="./FlameGraph"

if [ ! "$#" -eq 2 ]; then
	echo "Please provide two arguments!"
	exit 1
fi

if [ "$EUID" -ne 0 ]; then
	echo "Please run with sudo"
	exit 1
fi

if [ ! -d "$flmg" ]; then
	echo "$flmg doesn't exist!"
	echo "Run 'git clone https://github.com/brendangregg/FlameGraph.git'"
	exit 1
fi

if [ ! "$1" = "plot" ]; then
	perf record -F 99 -a -g -p $1 &
	perf_pid=$!

	sleep $2

	kill -INT $perf_pid
	wait $perf_pid
fi

perf script | ${flmg}/stackcollapse-perf.pl > out.stack
${flmg}/flamegraph.pl out.stack > flamegraph.svg
