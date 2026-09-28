#!/bin/bash

set -xe
code="$PWD"
opts=""
src=""
run_debug=""

opts="-ggdb -mfloat-abi=hard -Wall -Wextra -fjump-tables -nolibc --specs=nosys.specs -nostartfiles -I$code/inc/ -I$code/for_users/"

while [ $# -gt 0 ]; do
    case "$1" in
        f411)
            opts="-DBAD_PLATFORM_F411 -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -Tstm32f411ceu6.ld $opts" 
            src="$code/src/startup_stm32f411ceu6.c"
            ;;
        h562)
            opts="-DBAD_PLATFORM_H562 -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -Tstm32h562vgt6.ld $opts"
            src="$code/src/startup_stm32h562vgt6.c"
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

arm-none-eabi-gcc $opts -I"$code/inc" "$src" $code/tests/*.c -o build/tests.elf

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
