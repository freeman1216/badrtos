#define BAD_RTOS_IMPLEMENTATION
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#include "platform_include.h"

#ifdef BAD_RTOS_USE_MUTEX

bad_task_handle_t task1h;
bad_task_handle_t task2h;

bad_mutex_t mut;

void task1(void *unused)
{
    (void)unused;
    
    while(1)
    {
        mutex_take(&mut,0);
        task_yield();
        mutex_delete(&mut); 
        task_finish();
    }
}

void task2(void *unused)
{
    (void)unused;
    
    while(1)
    {
        bad_rtos_status_t status = mutex_take(&mut,0);
        
        if(status == BAD_RTOS_STATUS_DELETED)
        {
            task_finish();
        }
    }
}

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK2_STACK_SIZE 1024
#define TASK1_STACK_SIZE 1024
TASK_STATIC_STACK(task2, TASK2_STACK_SIZE);

bad_rtos_status_t bad_user_init()
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    bad_task_descr_t task1_descr = {
        .stack = 0,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    
    task1h = task_make(&task1_descr);
    
    ret = BAD_TASK_HANDLE_GET_ERROR(task1h);
    if(ret != BAD_RTOS_STATUS_OK)
        return ret;
    
    bad_task_descr_t task2_descr = {
        .stack = task2_stack,
        .stack_size = TASK2_STACK_SIZE,
        .entry = task2,
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    
    task2h = task_make(&task2_descr);
    
    ret = BAD_TASK_HANDLE_GET_ERROR(task2h);
    
    return ret;
}

#else

bad_rtos_status_t bad_user_init()
{
    return BAD_RTOS_STATUS_OK;
}

#endif

int __attribute__((noinline)) main()
{
    __platform_setup();
    
#ifdef BAD_RTOS_USE_MUTEX
    bad_rtos_start();
#endif
    
    while(1)
    {
        
    }
    
    return 0;
}
