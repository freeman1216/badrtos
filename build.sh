#!/bin/bash

set -xe
code="$PWD"

options="-ggdb -Wall -Wextra -fjump-tables -mfloat-abi=hard -nolibc --specs=nosys.specs -nostartfiles -I$code/inc/"
added_opts=""
optimisation=""
src=""
test_src=""
run_debug=""

while [ $# -gt 0 ]; do
    case "$1" in
        f411)
            added_opts="-DBAD_PLATFORM_F411 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Tstm32f411ceu6.ld" 
            src="$code/src/startup_stm32f411ceu6.c"
            ;;
        h562)
            added_opts="-DBAD_PLATFORM_H562 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -Tstm32h562vgt6.ld" 
            src="$code/src/startup_stm32h562vgt6.c"
            ;;
        opts)
            optimisation="-O2"
            ;;
        debug)
            run_debug="true"
            ;;
        time_frame)
            test_src="$code/tests/time_frame.c"
            ;;
        block_delay)
            test_src="$code/tests/block_delay.c"
            ;;
        mutex_block)
            test_src="$code/tests/mutex_block.c"
            ;;
        mutex_delay)
            test_src="$code/tests/mutex_delay.c"
            ;;
        mutex_delete)
            test_src="$code/tests/mutex_delete.c"
            ;;
        sem_block)
            test_src="$code/tests/sem_block.c"
            ;;
        sem_delay)
            test_src="$code/tests/sem_delay.c"
            ;;
        sem_delete)
            test_src="$code/tests/sem_delete.c"
            ;;
        sem_post_from_isr)
            test_src="$code/tests/sem_post_from_isr.c"
            ;;
        msgq)
            test_src="$code/tests/msgq.c"
            ;;
        msgq_post_from_isr)
            test_src="$code/tests/msgq_post_from_isr.c"
            ;;
        event_barrier_block)
            test_src="$code/tests/event_barrier_block.c"
            ;;
        event_barrier_delay)
            test_src="$code/tests/event_barrier_delay.c"
            ;;
        event_barrier_delete)
            test_src="$code/tests/event_barrier_delete.c"
            ;;
        event_barrier_post_from_isr)
            test_src="$code/tests/event_barrier_post_from_isr.c"
            ;;
        fpu)
            test_src="$code/tests/fpu.c"
            ;;
        buddy)
            test_src="$code/tests/buddy.c"
            ;;
        *)
            echo "Platform not supported"
            exit -1
            ;;
    esac
    shift
done

arm-none-eabi-gcc $added_opts $options $optimisation -I$code/inc/ $src $test_src  -o build/out.elf

if [ $run_debug = "true" ]; then 
    gf-svd build/out.elf \
        -ex "target extended-remote /dev/ttyBmpGdb" \
        -ex "monitor auto_scan"\
        -ex "attach 1"\
        -ex "load"\
        -ex "b main"\
        -ex "set mem inaccessible-by-default off"\
        -ex "run"
fi
