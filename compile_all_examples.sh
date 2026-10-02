#!/bin/bash

set -xe
code="$PWD"
src=""
run_debug=""
added_opts=""
optimisation=""
opts="-ggdb -mfloat-abi=hard -Wall -Wextra -fjump-tables -nolibc --specs=nosys.specs -nostartfiles -I$code/tests/test_platforms -I$code/inc/"
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

for file in examples/*.c; do
    out="build/$(basename "${file%.c}").elf"
    arm-none-eabi-gcc $opts -L$matched_dir $added_opts $optimisation $src "$code/$file" -o "$out"
    if [ "$run_debug" = "true" ]; then 
        gf-svd $out \
            -ex "target extended-remote /dev/ttyBmpGdb" \
            -ex "monitor auto_scan"\
            -ex "attach 1"\
            -ex "load"\
            -ex "b main"\
            -ex "set mem inaccessible-by-default off"\
            -ex "run"
    fi
done
