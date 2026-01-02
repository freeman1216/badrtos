# BADRTOS

## What is this?  
A lightweight header-only rtos scheduler for Cortex M4 mcus (in this case STM32F411CE) with usual RTOS primitives 
## Features
- Priority driven scheduler
- MPU support
- Mutexes, semaphores, message queues
- Software timers
- Dynamic memory allocation using buddy allocator
- Depends only on the linker file, startup code and some register definitions  
## How to use it  
1. Include the header and dependencies in your project.  
2. Define "BAD_RTOS_IMPLEMENTATION" and include the header 

```c
#define BAD_RTOS_IMPLEMENTATION
#include "bad_rtos.h"
```
3. Perform your setup in bad_user_setup
```c
void bad_user_setup(){
    bad_task_descr_t task1_descr = {
        .stack = 0,
        .stack_size = TASK1_STACK_SIZE,
        .dyn_stack = 1,
        .entry = task1,
        .regions = task1_regions,
        .region_count = MPU_REGIONS_SIZE(task1),
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    task1tcb = task_make(&task1_descr);
    bad_task_descr_t task2_descr = {
        .stack = task2_stack,
        .stack_size = TASK2_STACK_SIZE,
        .entry = task2,
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    task2tcb = task_make(&task2_descr);
}```
4. Start the scheduler
```c
bad_rtos_start();

```

## Notes
 - bad_rtos_start() can only be called in the translation unit where implementation was included
 - API is written with static and or zero initilisation in mind, no need to call stuff_init on everything, read the comments for more information
 - Lacks "volatile" compiler protection for now, so be aware of compiler optimisiations
 - If mpu is not used you can easily remove linker file and startup code dependencies by removing section attributes from definitions and adding attribute aligned(8) to stactic stacks macro and heaps and replace my headers with the CMIS one


You can freely modify or copy whatever you need.
