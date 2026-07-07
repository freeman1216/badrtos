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

for file in tests/*.c; do
    out="build/$(basename "${file%.c}").elf"
    arm-none-eabi-gcc $opts -I"$code/inc" "$file" -o "$out"
done
