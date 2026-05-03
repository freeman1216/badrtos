#define BAD_RTOS_ISR_TEST
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;
bad_event_barrier_t evb;
void task1(){
    volatile uint32_t unblocked = 0;
    while (1) {
        event_barrier_prime(&evb,1);
        event_barrier_wait(&evb,0);
        unblocked++;
    }
}

uint32_t shift;
void isr_test(){
    event_barrier_fire_from_isr(&evb,1UL << shift);
    shift = (shift+1) % 31;
}

#define TASK1_PRIORITY 1 
#define TASK1_STACK_SIZE 1024

TASK_STATIC_STACK(task1, TASK1_STACK_SIZE);
//
// START_TASK_MPU_REGIONS_DEFINITIONS(task2)
// #if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
//     DEFINE_STATIC_STACK_REGION(task2_stack,TASK2_STACK_SIZE)
// #endif
// END_TASK_MPU_REGIONS(task2)
//
void bad_user_setup(){
    bad_task_descr_t task1_descr = {
        .stack = task1_stack,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        //.regions = task1_regions,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1h = task_make(&task1_descr);
}


int __attribute__((noinline)) main(){
    __platform_setup();
    bad_rtos_start();
    //task_yield();
    while(1){

    
        
    }
    return 0;
}
