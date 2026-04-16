#include "platform_setup.h"
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;
bad_task_handle_t task2h;
bad_mutex_t mut;
void task1(){
    while (1) {
        mutex_take(&mut,0);
        task_yield();
        mutex_delete(&mut); 
        task_finish();
    }
}

void task2(){
    while (1) {
        bad_rtos_status_t status;
        status = mutex_take(&mut,0);
        if(status == BAD_RTOS_STATUS_DELETED){
            task_finish();
        }
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
        .ticks_to_change = 500,
        .regions = task2_regions,
        .base_priority = TASK2_PRIORITY
    };
    task2h = task_make(&task2_descr);
}


int __attribute__((noinline)) main(){
    __DISABLE_INTERUPTS;
    __main_clock_setup();
    __periph_setup();
    __ENABLE_INTERUPTS;
    bad_rtos_start();
    //task_yield();
    while(1){

    
        
    }
    return 0;
}
