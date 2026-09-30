#!/bin/bash

set -xe
code="$PWD"

options="-ggdb -Wall -Wextra -fjump-tables -mfloat-abi=hard -nolibc --specs=nosys.specs -nostartfiles -I$code/tests -I$code/inc/"
added_opts=""
optimisation=""
src=""
example_src=""
run_debug=""

while [ $# -gt 0 ]; do
    case "$1" in
        f411)
            added_opts="-DBAD_PLATFORM_F411 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Tstm32f411ceu6.ld" 
            src="$code/src/startup_stm32f411ceu6.c $code/tests/test_platforms/platform_setup_f411.c"
            ;;
        h562)
            added_opts="-DBAD_PLATFORM_H562 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -Tstm32h562vgt6.ld" 
            src="$code/src/startup_stm32h562vgt6.c $code/tests/test_platforms/platform_setup_h562.c"
            ;;
        opts)
            optimisation="-O2"
            ;;
        debug)
            run_debug="true"
            ;;
        time_frame)
            example_src="$code/examples/time_frame.c"
            ;;
        block_delay)
            example_src="$code/examples/block_delay.c"
            ;;
        mutex_block)
            example_src="$code/examples/mutex_block.c"
            ;;
        mutex_delay)
            example_src="$code/examples/mutex_delay.c"
            ;;
        mutex_delete)
            example_src="$code/examples/mutex_delete.c"
            ;;
        sem_block)
            example_src="$code/examples/sem_block.c"
            ;;
        sem_delay)
            example_src="$code/examples/sem_delay.c"
            ;;
        sem_delete)
            example_src="$code/examples/sem_delete.c"
            ;;
        sem_post_from_isr)
            example_src="$code/examples/sem_post_from_isr.c"
            ;;
        msgq)
            example_src="$code/examples/msgq.c"
            ;;
        msgq_post_from_isr)
            example_src="$code/examples/msgq_post_from_isr.c"
            ;;
        event_barrier_block)
            example_src="$code/examples/event_barrier_block.c"
            ;;
        event_barrier_delay)
            example_src="$code/examples/event_barrier_delay.c"
            ;;
        event_barrier_delete)
            example_src="$code/examples/event_barrier_delete.c"
            ;;
        event_barrier_post_from_isr)
            example_src="$code/examples/event_barrier_post_from_isr.c"
            ;;
        fpu)
            example_src="$code/examples/fpu.c"
            ;;
        buddy)
            example_src="$code/examples/buddy.c"
            ;;
        *)
            echo "Platform not supported"
            exit -1
            ;;
    esac
    shift
done

arm-none-eabi-gcc $added_opts $options $optimisation -I$code/inc/ $src $example_src  -o build/out.elf

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
