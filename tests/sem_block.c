#include "platform_setup.h"
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;
bad_task_handle_t task2h;
bad_sem_t sem;
void task1(){
    while (1) {
        sem_take(&sem,0);
        task_yield();
        sem_put(&sem);
        task_yield();
    }
}

void task2(){
    while (1) {
        sem_take(&sem,0);
        task_yield();
        sem_put(&sem);
        task_yield();
    }
}
void task3(){
    while (1) {
        sem_take(&sem,0);
        task_yield();
        sem_put(&sem);
        task_yield();
    }
}

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1
#define TASK1_STACK_SIZE 1024
#define TASK2_STACK_SIZE 1024
#define TASK3_STACK_SIZE 1024
TASK_STATIC_STACK(task2, TASK2_STACK_SIZE);
TASK_STATIC_STACK(task3, TASK2_STACK_SIZE);

START_TASK_MPU_REGIONS_DEFINITIONS(task2)
#if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
    DEFINE_STATIC_STACK_REGION(task2_stack,TASK2_STACK_SIZE)
#endif
END_TASK_MPU_REGIONS(task2)

START_TASK_MPU_REGIONS_DEFINITIONS(task3)
#if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
    DEFINE_STATIC_STACK_REGION(task3_stack,TASK3_STACK_SIZE)
#endif
END_TASK_MPU_REGIONS(task3)

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
        .stack = task3_stack,
        .stack_size = TASK3_STACK_SIZE,
        .entry = task3,
        .regions = task3_regions,
        .ticks_to_change = 500,
        .base_priority = TASK3_PRIORITY
    };
    task2h = task_make(&task3_descr);
    sem_init(&sem, 2);
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
