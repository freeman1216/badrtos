CC = arm-none-eabi-gcc
CFLAGS =  -ggdb -Wall -Wextra -fjump-tables -mfloat-abi=hard
F411_CFLAGS = -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -DBAD_PLATFORM_F411
H562_CFLAGS = -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -DBAD_PLATFORM_H562
LDFLAGS = -nolibc --specs=nosys.specs -nostartfiles
F411_LDFLAGS = -Tstm32f411ceu6.ld
H562_LDFLAGS = -Tstm32h562vgt6.ld 

INCLUDES = -Iinc/

SRC_DIR = src
BUILD_DIR = build

F411_SOURCES = $(SRC_DIR)/startup_stm32f411ceu6.c tests/platform_setup_f411.c
H562_SOURCES = $(SRC_DIR)/startup_stm32h562vgt6.c tests/platform_setup_h562.c

PLATFORM_UPPER := $(shell echo $(PLATFORM) | tr a-z A-Z)

ifndef PLATFORM
  $(error provide a platform like `make PLATFORM=h562 main`)
endif


SOURCES = $($(PLATFORM_UPPER)_SOURCES)
LDFLAGS += $($(PLATFORM_UPPER)_LDFLAGS)
CFLAGS	+= $($(PLATFORM_UPPER)_CFLAGS)


MAIN_SRC = $(SOURCES) $(SRC_DIR)/main.c
TIME_FRAME_SRC = $(SOURCES) tests/time_frame.c
BLOCK_DELAY_SRC = $(SOURCES) tests/block_delay.c
MUTEX_BLOCK_SRC = $(SOURCES) tests/mutex_block.c
MUTEX_DELAY_SRC = $(SOURCES) tests/mutex_delay.c
MUTEX_DELETE_SRC = $(SOURCES) tests/mutex_delete.c
SEM_BLOCK_SRC = $(SOURCES) tests/sem_block.c
SEM_DELAY_SRC = $(SOURCES) tests/sem_delay.c
SEM_DELETE_SRC = $(SOURCES) tests/sem_delete.c
MSGQ_SRC = $(SOURCES) tests/msgq.c
NBSEM_DELETE_SRC = $(SOURCES) tests/nbsem_delete.c
BUDDY_SRC = $(SOURCES) tests/buddy.c
FPU_SRC = $(SOURCES) tests/fpu.c

MAIN_BIN = $(BUILD_DIR)/main.elf
TIME_FRAME_BIN = $(BUILD_DIR)/time_frame.elf
BLOCK_DELAY_BIN = $(BUILD_DIR)/block_delay.elf
MUTEX_BLOCK_BIN = $(BUILD_DIR)/mutex_block.elf
MUTEX_DELAY_BIN = $(BUILD_DIR)/mutex_delay.elf
MUTEX_DELETE_BIN = $(BUILD_DIR)/mutex_delete.elf
SEM_BLOCK_BIN = $(BUILD_DIR)/sem_block.elf
SEM_DELAY_BIN = $(BUILD_DIR)/sem_delay.elf
SEM_DELETE_BIN = $(BUILD_DIR)/sem_delete.elf
MSGQ_BIN = $(BUILD_DIR)/msgq.elf
NBSEM_DELETE_BIN = $(BUILD_DIR)/nbsem_delete.elf
BUDDY_BIN = $(BUILD_DIR)/buddy.elf
FPU_BIN = $(BUILD_DIR)/fpu.elf

PRIMARY_GOAL := $(firstword $(MAKECMDGOALS))

PRIMARY_GOAL_UPPER := $(shell echo $(PRIMARY_GOAL) | tr a-z A-Z)

CURRBIN := $($(PRIMARY_GOAL_UPPER)_BIN)

###############
# Build rules #
###############

.DEFAULT_GOAL := help

.PHONY: help
help:
	$(error No target specified. Example: make exti or make exti debug)


$(MAIN_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(MAIN_SRC) -o $@

$(TIME_FRAME_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(TIME_FRAME_SRC) -o $@

$(BLOCK_DELAY_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(BLOCK_DELAY_SRC) -o $@

$(MUTEX_BLOCK_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(MUTEX_BLOCK_SRC) -o $@

$(MUTEX_DELETE_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(MUTEX_DELETE_SRC) -o $@

$(MUTEX_DELAY_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(MUTEX_DELAY_SRC) -o $@

$(SEM_BLOCK_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SEM_BLOCK_SRC) -o $@

$(SEM_DELAY_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SEM_DELAY_SRC) -o $@

$(SEM_DELETE_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SEM_DELETE_SRC) -o $@

$(MSGQ_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(MSGQ_SRC) -o $@

$(NBSEM_DELETE_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(NBSEM_DELETE_SRC) -o $@

$(BUDDY_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(BUDDY_SRC) -o $@

$(FPU_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(FPU_SRC) -o $@

.PHONY: main
main: clean $(MAIN_BIN)

.PHONY: time_frame
time_frame: clean $(TIME_FRAME_BIN)

.PHONY: block_delay
block_delay: clean $(BLOCK_DELAY_BIN)

.PHONY: mutex_block
mutex_block: clean $(MUTEX_BLOCK_BIN)

.PHONY: mutex_delay
mutex_delay: clean $(MUTEX_DELAY_BIN)

.PHONY: mutex_delete
mutex_delete: clean $(MUTEX_DELETE_BIN)

.PHONY: sem_block
sem_block: clean $(SEM_BLOCK_BIN)

.PHONY: sem_delay
sem_delay: clean $(SEM_DELAY_BIN)

.PHONY: sem_delete
sem_delete: clean $(SEM_DELETE_BIN)

.PHONY: msgq
msgq: clean $(MSGQ_BIN)

.PHONY: nbsem_delete
nbsem_delete: clean $(NBSEM_DELETE_BIN)

.PHONY:	buddy
buddy: clean $(BUDDY_BIN)

.PHONY:	fpu
fpu: clean $(FPU_BIN)

.PHONY: debug
debug:
ifeq ($(CURRBIN),)
	$(error "debug must follow a build target, e.g. `make exti debug`")
endif

	gf2 $(CURRBIN) \
		-ex "target extended-remote /dev/ttyBmpGdb" \
		-ex "monitor auto_scan"\
		-ex "attach 1"\
		-ex "load"\
		-ex "b main"\
		-ex "set mem inaccessible-by-default off"\
		-ex "run"


###############
# Clean       #
###############
.PHONY: clean
clean:
	rm -f $(BUILD_DIR)/*.elf

###############
# Build dir   #
###############
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
