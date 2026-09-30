#!/bin/bash

set -xe
code="$PWD"
opts=""
src=""
run_debug=""
added_opts=""
optimisation=""
opts="-ggdb -mfloat-abi=hard -Wall -Wextra -fjump-tables -nolibc --specs=nosys.specs -nostartfiles  -I$code/tests/ -I$code/inc/"

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
        *)
            echo "Option not supported"
            exit -1
            ;;
    esac
    shift
done

for file in examples/*.c; do
    out="build/$(basename "${file%.c}").elf"
    arm-none-eabi-gcc $opts $added_opts $optimisation -I"$code/inc" $src "$code/$file" -o "$out"
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
