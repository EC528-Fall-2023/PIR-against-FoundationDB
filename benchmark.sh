#!/bin/bash

program=$1
iterations=$2

test_startup()
{
	make clean -s
	SEED=1 BYTES="$1" BLOCKS="$2" LEVELS="$3" make startup -s
	for ((i = 1; i <= iterations; i++)); do
		sync && echo 3 > /proc/sys/vm/drop_caches

		taskset -c 0 $PWD/bin/server &
		SERVER_PID=$!

		sleep 2

		taskset -c 1 $PWD/bin/"$program" &

		wait $!
		wait $SERVER_PID

		$PWD/bin/clear_fdb

		sleep 2
	done
}

test_without()
{
	make clean -s
	make without -s

	for ((i = 1; i <= iterations; i++)); do
		$PWD/bin/without -w austin=good

		$PWD/bin/without -r austin
	done
}

if [ "$program" == "startup" ]; then
	# default parameters
	echo "start default"
	test_startup 1024 1 8

	# vary BYTES
	echo "start varying bytes"
	test_startup 16384 1 8
	test_startup 32768 1 8
	test_startup 65536 1 8

	# vary BLOCKS
	echo "start varying blocks"
	test_startup 1024 16 8
	test_startup 1024 32 8
	test_startup 1024 48 8

	# vary LEVELS
	echo "start varying levels"
	test_startup 1024 1 12
	test_startup 1024 1 16
	test_startup 1024 1 20

	echo "end startup benchmark"
elif [ "$program" == "fdb" ]; then
	# default parameters
	echo "start default"
	test_other 1024 1 8

	# vary BYTES
	echo "start varying bytes"
	test_other 16384 1 8
	test_other 32768 1 8
	test_other 65536 1 8

	# vary BLOCKS
	echo "start varying blocks"
	test_other 1024 16 8
	test_other 1024 32 8
	test_other 1024 48 8

	# vary LEVELS
	echo "start varying levels"
	test_other 1024 1 12
	test_other 1024 1 16
	test_other 1024 1 20

	# vary fullness
	echo "start full tree"
	iterations=255
	test_other 1024 1 8

	echo "end fdb benchmark"
else
	echo "start without"
	
	test_without

	echo "end"
fi


