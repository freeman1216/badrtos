#define BAD_RTOS_ISR_TEST
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;

bad_sem_t sem = {.init_flag = 1};
void task1(void *unused){
    (void)unused;
    while (1) {
        bad_rtos_status_t res = sem_take(&sem,0);
    }
}

void isr_test(){
    sem_put_from_isr(&sem);
}

#define TASK1_PRIORITY 1 
#define TASK1_STACK_SIZE 1024

TASK_STATIC_STACK(task1, TASK1_STACK_SIZE);

#ifdef BAD_RTOS_USE_MPU
START_TASK_MPU_REGIONS_DEFINITIONS(task1)
#if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
DEFINE_STATIC_STACK_REGION(task1_stack,TASK1_STACK_SIZE)
#endif
END_TASK_MPU_REGIONS(task1)
#endif

void bad_user_init(){
    bad_task_descr_t task1_descr = {
        .stack = task1_stack,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
#ifdef BAD_RTOS_USE_MPU
        .regions = task1_regions,
#endif
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
