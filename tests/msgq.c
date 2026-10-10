#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MSGQ

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

MSGQ_DEFINE_STATIC(task1q, 16);
MSGQ_DEFINE(task34, 16);
static void task1(void *unused)
{
    (void)unused;
    
    msgq_acquire(&task1q);
    
    volatile uint32_t sig0 = 0;
    volatile uint32_t sig1 = 0;
    volatile uint32_t sig2 = 0;
    volatile uint32_t sig3 = 0;
    
    bad_msg_block_t msg = {0};
    msgq_pull_msg(&task1q, &msg,0);
    
    do
    {
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
    while(msgq_pull_msg(&task1q, &msg, -1) != BAD_RTOS_STATUS_WOULD_BLOCK);
    
    BAD_ASSERT(sig0 == 4 && sig1 == 4 && sig2 == 4 && sig3 == 4, "Message counts mismatch");
    
    msgq_release(&task1q);
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    u32 sig = 0;
    
    while(msgq_post_msg(&task1q,sig,0,-1) != BAD_RTOS_STATUS_WOULD_BLOCK)
    {
        sig = (sig + 1) & 0x3;
    }
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,msgq) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 1,
    .test_name = "Message queue"
};

#endif