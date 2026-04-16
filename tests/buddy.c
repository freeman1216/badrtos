#include "platform_setup.h"
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;
bad_task_handle_t task2h;
bad_task_handle_t task3h;
uint32_t callback_hit;
uint32_t unblocked;
void cb(bad_task_handle_t unused0, void* unused1){
    UNUSED(unused0);
    UNUSED(unused1);
    callback_hit++;
} 
void task1(){
    while (1) {
        task_delay(200, cb, 0);
        uint32_t *arr = user_alloc(16);
        task_block();
        user_free(arr,16);
    }
}

void task2(){
    while (1) {
        bad_rtos_status_t status;
        status = task_unblock(task1h);
        if(status == BAD_RTOS_STATUS_OK){
            unblocked++;
            task_yield();
        }else{
            task_delay(200, 0, 0);
        }
    }
}

void task3(){
    while (1) {
    
    }
}

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK2_STACK_SIZE 1024
#define TASK1_STACK_SIZE 1024
TASK_STATIC_STACK(task2, TASK2_STACK_SIZE);

START_TASK_MPU_REGIONS_DEFINITIONS(task2)
#if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
    DEFINE_STATIC_STACK_REGION(task2_stack,TASK2_STACK_SIZE)
#endif
END_TASK_MPU_REGIONS(task2)

void bad_user_setup(){
    bad_task_descr_t task1_descr = {
        .stack = 0,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        //.regions = task1_regions,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1h = task_make(&task1_descr);
    bad_task_descr_t task2_descr = {
        .stack = task2_stack,
        .stack_size = TASK2_STACK_SIZE,
        .entry = task2,
        .regions = task2_regions,
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    task2h = task_make(&task2_descr);
    bad_task_descr_t task3_descr = {
        .stack = 0,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task3,
        //.regions = task1_regions,
        .ticks_to_change = 500,
        .base_priority = IDLE_TASK_PRIO + 1
    };
    task3h = task_make(&task3_descr);
}


int __attribute__((noinline)) main(){
    __DISABLE_INTERUPTS;
    __platform_setup();
    __ENABLE_INTERUPTS;
    bad_rtos_start();
    //task_yield();
    while(1){

    
        
    }
    return 0;
}
