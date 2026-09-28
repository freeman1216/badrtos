#define BAD_RTOS_ISR_TEST
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

bad_task_handle_t task1h;

bad_sem_t sem;

void task1(void *unused)
{
    (void)unused;
    
    while(1)
    {
        volatile bad_rtos_status_t res = sem_take(&sem,0);
    }
}

void isr_test()
{
    sem_put_from_isr(&sem);
}

#define TASK1_PRIORITY 1 
#define TASK1_STACK_SIZE 1024

TASK_STATIC_STACK(task1, TASK1_STACK_SIZE);

bad_rtos_status_t bad_user_init()
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    bad_task_descr_t task1_descr = {
        .stack = task1_stack,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1h = task_make(&task1_descr);
    
    ret = BAD_TASK_HANDLE_GET_ERROR(task1h);
    
    sem_init(&sem,0);
    
    return ret;
}

#else

void isr_test()
{
    
}

bad_rtos_status_t bad_user_init()
{
    return BAD_RTOS_STATUS_OK;
}

#endif

int __attribute__((noinline)) main()
{
    __platform_setup();
    
#ifdef BAD_RTOS_USE_SEMAPHORE
    bad_rtos_start();
#endif
    
    while(1)
    {
        
    }
    
    return 0;
}
