#!/bin/bash

set -xe
code="$PWD"
opts="-ggdb -Wall -Wextra -fjump-tables -mfloat-abi=hard -nolibc --specs=nosys.specs -nostartfiles -I$code/tests/test_platforms -I$code/inc/"
added_opts=""
optimisation=""
src=""
example_src=""
run_debug=""
platform=""
matched_dir=""

while [ $# -gt 0 ]; do
    case "$1" in
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
            platform="$1"
            ;;
    esac
    shift
done

if [ "$platform" = "" ]; then
    echo "Error : No platform provided"
    exit 1
fi

for dir in "$code/tests/test_platforms"/*"$platform"*; do
    if [ -d "$dir" ]; then
        matched_dir="$dir"
        break
    fi
done

if [ -n "$matched_dir" ]; then
    added_opts="$(cat "$matched_dir/platform.cflags")"
    src="$matched_dir/*.c"
else
    echo "Error: Platform or option '$platform' not supported"
    exit 1
fi

arm-none-eabi-gcc $opts -L$matched_dir $added_opts $optimisation $src $example_src -o build/out.elf

if [ "$run_debug" = "true" ]; then 
    gf-svd build/out.elf \
        -ex "target extended-remote /dev/ttyBmpGdb" \
        -ex "monitor auto_scan"\
        -ex "attach 1"\
        -ex "load"\
        -ex "b main"\
        -ex "set mem inaccessible-by-default off"\
        -ex "run"
fi
