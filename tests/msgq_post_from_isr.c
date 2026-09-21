#define BAD_RTOS_ISR_TEST
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

#ifdef BAD_RTOS_USE_MSGQ

bad_task_handle_t task1h;

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

uint32_t sig = 0;
void isr_test()
{
    msgq_post_msg_from_isr(&task1q,sig,0);
    sig = (sig + 1) & 0x3;
}

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK2_STACK_SIZE 1024
#define TASK1_STACK_SIZE 1024

TASK_STATIC_STACK(task1, TASK1_STACK_SIZE);

bad_rtos_status_t bad_user_init()
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    bad_task_descr_t task1_descr = {
        .stack = task1_stack,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        .assigned_msgq = &task1q,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    
    task1h = task_make(&task1_descr);
    ret = BAD_TASK_HANDLE_GET_ERROR(task1h);
    
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
    
#ifdef BAD_RTOS_USE_MSGQ
    bad_rtos_start();
#endif
    
    while(1)
    {
        
    }
    return 0;
}
