#!/bin/bash

set -xe
code="$PWD"
src=""
run_debug=""
added_opts=""
optimisation=""
opts="-ggdb -mfloat-abi=hard -Wall -Wextra -fjump-tables -nolibc --specs=nosys.specs -nostartfiles -I$code/inc/ -I$code/tests/test_platforms/"
matched_dir=""
platform=""

while [ $# -gt 0 ]; do
    case "$1" in
		opts)
            optimisation="-O2"
			;;
        debug)
            run_debug="true"
            ;;
        *)
            platform="$1"
            ;;
    esac
    shift
done

if [ "$platform" = "" ]; then
    echo "Error: No platform provided"
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

arm-none-eabi-gcc $opts -L$matched_dir $added_opts $optimisation $src $code/tests/*.c -o build/tests.elf

if ! gdb build/tests.elf \
        -batch \
        -ex "target extended-remote /dev/ttyBmpGdb" \
        -ex "monitor auto_scan"\
        -ex "attach 1"\
        -ex "load"\
        -ex "set mem inaccessible-by-default off"\
        -x "auto_tests.gdb"
then 
    if [ "$run_debug" = "true" ]; then 
        gf-svd build/tests.elf \
            -ex "target extended-remote /dev/ttyBmpGdb" \
            -ex "monitor auto_scan"\
            -ex "attach 1"\
            -ex "b main"\
            -ex "set mem inaccessible-by-default off"\
            -ex "run"
        exit 1
    fi
fi
