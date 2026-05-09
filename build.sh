#!/bin/bash

set -xe
code="$PWD"
opts=""
src=""

opts="-ggdb -Wall -Wextra -fjump-tables -mfloat-abi=hard -nolibc --specs=nosys.specs -nostartfiles  -I$code/inc/"

case "$1" in
	f411)
		opts="-DBAD_PLATFORM_F411 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Tstm32f411ceu6.ld $opts" 
		src="$code/src/startup_stm32f411ceu6.c"
		;;
	h562)
		opts="-DBAD_PLATFORM_H562 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -Tstm32h562vgt6.ld $opts"
		src="$code/src/startup_stm32h562vgt6.c"
		;;
	*)
		echo "Platform not supported"
		exit -1
		;;
esac

case "$2" in
	main)
		src="$code/src/main.c $src"
		;;
	time_frame)
		src="$code/tests/time_frame.c $src"
		;;
	block_delay)
		src="$code/tests/block_delay.c $src"
		;;
	mutex_block)
		src="$code/tests/mutex_block.c $src"
		;;
	mutex_delay)
		src="$code/tests/mutex_delay.c $src"
		;;
	mutex_delete)
		src="$code/tests/mutex_delete.c $src"
		;;
	sem_block)
		src="$code/tests/sem_block.c $src"
		;;
	sem_delay)
		src="$code/tests/sem_delay.c $src"
		;;
	sem_delete)
		src="$code/tests/sem_delete.c $src"
		;;
	sem_post_from_isr)
		src="$code/tests/sem_post_from_isr.c $src"
		;;
	msgq)
		src="$code/tests/msgq.c $src"
		;;
	msgq_post_from_isr)
		src="$code/tests/msgq_post_from_isr.c $src"
		;;
	event_barrier_block)
		src="$code/tests/event_barrier_block.c $src"
		;;
	event_barrier_delay)
		src="$code/tests/event_barrier_delay.c $src"
		;;
	event_barrier_delete)
		src="$code/tests/event_barrier_delete.c $src"
		;;
	event_barrier_post_from_isr)
		src="$code/tests/event_barrier_post_from_isr.c $src"
		;;
	fpu)
		src="$code/tests/fpu.c $src"
		;;
	buddy)
		src="$code/tests/fpu.c $src"
		;;
	*)
		echo "No such target"
		exit -1

		;;
esac

cd . > /dev/null
arm-none-eabi-gcc $opts -I$code/inc/ $src -o build/out.elf
cd $code > /dev/null

if [ "$3" = "debug" ]; then 
	gf2 build/out.elf \
		-ex "target extended-remote /dev/ttyBmpGdb" \
		-ex "monitor auto_scan"\
		-ex "attach 1"\
		-ex "load"\
		-ex "b main"\
		-ex "set mem inaccessible-by-default off"\
		-ex "run"
fi
