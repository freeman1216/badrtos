CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
CFLAGS = -ggdb -Wall -Wextra -fjump-tables -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -fno-strict-aliasing 
LDFLAGS = -Tstm32f411ceu6.ld -nolibc --specs=nosys.specs -nostartfiles  
INCLUDES = -Iinc/ -Iinc/driver/ -Iinc/rtos

SRC_DIR = src
BUILD_DIR = build

SOURCES = $(SRC_DIR)/startup_stm32f411ceu6.c 

MAIN_SRC = $(SOURCES) $(SRC_DIR)/main.c
MAIN_BIN = $(BUILD_DIR)/badrtos.elf

MAIN_SRC = $(SOURCES) $(SRC_DIR)/main.c
TIME_FRAME_SRC = $(SOURCES) tests/time_frame.c
BLOCK_DELAY_SRC = $(SOURCES) tests/block_delay.c
MUTEX_BLOCK_SRC = $(SOURCES) tests/mutex_block.c
MUTEX_DELETE_SRC = $(SOURCES) tests/mutex_delete.c
SEM_BLOCK_SRC = $(SOURCES) tests/sem_block.c
SEM_DELETE_SRC = $(SOURCES) tests/sem_delete.c
MSGQ_SRC = $(SOURCES) tests/msgq.c
NBSEM_DELETE_SRC = $(SOURCES) tests/nbsem_delete.c

MAIN_BIN = $(BUILD_DIR)/main.elf
TIME_FRAME_BIN = $(BUILD_DIR)/time_frame.elf
BLOCK_DELAY_BIN = $(BUILD_DIR)/block_delay.elf
MUTEX_BLOCK_BIN = $(BUILD_DIR)/mutex_block.elf
MUTEX_DELETE_BIN = $(BUILD_DIR)/mutex_delete.elf
SEM_BLOCK_BIN = $(BUILD_DIR)/sem_block.elf
SEM_DELETE_BIN = $(BUILD_DIR)/sem_delete.elf
MSGQ_BIN = $(BUILD_DIR)/msgq.elf
NBSEM_DELETE_BIN = $(BUILD_DIR)/nbsem_delete.elf

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

$(SEM_BLOCK_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SEM_BLOCK_SRC) -o $@

$(SEM_DELETE_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(SEM_DELETE_SRC) -o $@

$(MSGQ_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(MSGQ_SRC) -o $@

$(NBSEM_DELETE_BIN): $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDES) $(NBSEM_DELETE_SRC) -o $@

.PHONY: main
main: clean $(MAIN_BIN)

.PHONY: time_frame
time_frame: clean $(TIME_FRAME_BIN)

.PHONY: block_delay
block_delay: clean $(BLOCK_DELAY_BIN)

.PHONY: mutex_block
mutex_block: clean $(MUTEX_BLOCK_BIN)

.PHONY: mutex_delete
mutex_delete: clean $(MUTEX_DELETE_BIN)

.PHONY: sem_block
sem_block: clean $(SEM_BLOCK_BIN)

.PHONY: sem_delete
sem_delete: clean $(SEM_DELETE_BIN)

.PHONY: msgq
msgq: clean $(MSGQ_BIN)

.PHONY: nbsem_delete
nbsem_delete: clean $(NBSEM_DELETE_BIN)

.PHONY: debug
debug:
ifeq ($(CURRBIN),)
	$(error "debug must follow a build target, e.g. `make exti debug`")
endif

	@echo "Flashing $(CURRBIN)..."
	openocd -f /usr/share/openocd/scripts/interface/stlink.cfg \
	-f /usr/share/openocd/scripts/target/stm32f4x.cfg \
		-c "program $(CURRBIN) reset exit"

	@echo "Starting OpenOCD server..."
	openocd -f /usr/share/openocd/scripts/interface/stlink.cfg \
	-f /usr/share/openocd/scripts/target/stm32f4x.cfg & \
	gf2 $(CURRBIN) \
		-ex "target remote localhost:3333" \
		-ex "monitor reset halt" 
	
	pkill openocd




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
