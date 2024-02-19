#!/bin/bash
#
# Before you run vmpte, memory overcommit needs
# to be enabled since we are allocating huge
# amounts of the address space.
#
# THP needs to also be set to madvise for controlling
# page table granilarity. It's not a must, though.
#
# Author: Karim Manaouil <k.manaouil@gmail.com>
# 		Edinburgh University 23-24
#

trap finalize INT


if [ "$EUID" -ne 0 ]; then
	echo "Please run with sudo"
        exit 1
fi

overcommit=$(cat /proc/sys/vm/overcommit_memory)

thp=$(cat /sys/kernel/mm/transparent_hugepage/enabled)
thp=$(echo $thp | grep -Po "\[.*\]" | sed -E -e 's/\[|\]//g')

function finalize() {
	echo "Reverting kernel config"
	echo "$overcommit" > /proc/sys/vm/overcommit_memory
	echo "$thp" > /sys/kernel/mm/transparent_hugepage/enabled
	exit 0
}

echo "overcommit=$overcommit changing to 1"
echo "thp=$thp changing to madvise"

echo 1 > /proc/sys/vm/overcommit_memory
echo madvise > /sys/kernel/mm/transparent_hugepage/enabled

./vmpte
finalize
