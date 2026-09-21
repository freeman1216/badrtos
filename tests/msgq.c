#define BAD_RTOS_IMPLEMENTATION
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#include "platform_include.h"

#ifdef BAD_RTOS_USE_MSGQ

bad_task_handle_t task1h;
bad_task_handle_t task2h;

MSGQ_STATIC_INIT(task1q, 16);

void task1(void *unused)
{
    (void)unused;
    
    volatile uint32_t sig0 = 0;
    volatile uint32_t sig1 = 0;
    volatile uint32_t sig2 = 0;
    volatile uint32_t sig3 = 0;
    
    while(1)
    {
        bad_msg_block_t msg = {0};
        msgq_pull_msg(&task1q, &msg,0);
        
        switch(msg.signal)
        {
            case 0:
            {
                sig0++;
            }break;
            
            case 1:
            {
                sig1++;
            }break;
            
            case 2:
            {
                sig2++;
            }break;
            
            case 3:
            {
                sig3++;
            }break;
        }
    }
}


void task2(void *unused)
{
    (void)unused;
    static volatile uint32_t sig = 0;
    while(1)
    {
        msgq_post_msg(&task1q, sig, 0,0);
        sig = (sig + 1) & 0x3;
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
        .assigned_msgq = &task1q,
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
