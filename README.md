# BADRTOS

Lightweight header-only RTOS scheduler for Cortex-M4/M33 MCUs. Tested on STM32F411CE and STM32H562VG.

## Features

- Priority driven scheduler
- No ISR locks in the kernel
- MPU support
- Mutexes, semaphores, message queues, event barriers
- Software timers
- Dynamic memory allocation using a buddy allocator and pools
- IRQ ownership API

## Repository layout

| Path | Contents |
|------|----------|
| `for_users/` | Joined single-file headers (`badrtos_*arch*.h`), those are the one to use |
| `examples/` | Example projects |
| `test/` | Automated tests |
| `run_tests.sh` | Runs the automated tests |

## Usage

1. Add the header 
2. Add the nessesary regions to your linker file (described at the top of the header)
3. Setup the tick, set its interrupt priority to the lowest available on your MCU
4. Setup nessesary defines in the header (isr count, flash/ram size, isr names, prio bits)
5. In exactly one translation unit, define `BAD_RTOS_IMPLEMENTATION` before including the header:

```c
#define BAD_RTOS_IMPLEMENTATION
#include "bad_rtos.h"
```

6. Create your initial tasks and objects in `bad_user_init`. Return `BAD_RTOS_STATUS_OK` to start the scheduler.

```c
bad_rtos_status_t bad_user_init(void){
    bad_task_descr_t task1_descr = {
        .stack = 0,                      // 0 = allocate from kernel heap
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        .regions = task1_regions,        // MPU regions, zero terminated
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1h = task_make(&task1_descr);

    bad_task_descr_t task2_descr = {
        .stack = task2_stack,            // static stack, see TASK_STATIC_STACK
        .stack_size = TASK2_STACK_SIZE,
        .entry = task2,
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    task2h = task_make(&task2_descr);

    return BAD_RTOS_STATUS_OK;
}
```

7. Start the scheduler:

```c
bad_rtos_start();
```

## Configuration

Set at the top of the header. Comment out to disable.

| Macro | Effect |
|-------|--------|
| `BAD_RTOS_USE_KHEAP` | Kernel heap (buddy allocator) |
| `BAD_RTOS_USE_MUTEX` | Mutexes |
| `BAD_RTOS_USE_SEMAPHORE` | Semaphores |
| `BAD_RTOS_USE_MSGQ` | Message queues |
| `BAD_RTOS_USE_EVENT_BARRIER` | Event barriers |
| `BAD_RTOS_USE_MPU` | MPU support |
| `BAD_RTOS_USE_FPU` | FPU support |

Other settings: `BAD_RTOS_MAX_TASKS`, `BAD_RTOS_PRIO_BITS`, `BAD_RTOS_IRQ_COUNT`, `BAD_RTOS_GLOBAL_POOL_SIZE`, and the flash/RAM address and size macros used for the MPU default regions.

Full description of every function is in the comment block at the top of the header.

## Tests

Automated tests are in `test/` and are run with:

```sh
./run_tests.sh
```

## Notes

- Set the tick timer interrupt priority to 15.

## License

You can freely modify or copy whatever you need
