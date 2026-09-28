#define BAD_RTOS_IMPLEMENTATION
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#include "platform_include.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

bad_task_handle_t task1h;
bad_task_handle_t task2h;
bad_task_handle_t task3h;

bad_sem_t sem;

void task1(void *unused)
{
    (void)unused;
    while(1)
    {
        sem_take(&sem,0);
        task_yield();
        sem_put(&sem);
        task_yield();
    }
}

void task2(void *unused)
{
    (void)unused;
    while(1)
    {
        sem_take(&sem,0);
        task_yield();
        sem_put(&sem);
        task_yield();
    }
}

void task3(void *unused)
{
    (void)unused;
    while(1)
    {
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
    if(ret != BAD_RTOS_STATUS_OK)
        return ret;
    
    bad_task_descr_t task3_descr = {
        .stack = task3_stack,
        .stack_size = TASK3_STACK_SIZE,
        .entry = task3,
        .ticks_to_change = 500,
        .base_priority = TASK3_PRIORITY
    };
    
    task3h = task_make(&task3_descr);
    
    ret = BAD_TASK_HANDLE_GET_ERROR(task3h);
    
    sem_init(&sem,1);
    
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
    
#ifdef BAD_RTOS_USE_SEMAPHORE
    bad_rtos_start();
#endif
    
    while(1)
    {
        
    }
    
    return 0;
}
