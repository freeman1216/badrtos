#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MSGQ

#define TASK1_PRIORITY 1 

#define MSGQ_SIGNAL 0x1337322
#define MSGQ_ARG 0xDEADBEEF

MSGQ_DEFINE_STATIC(task1q,16);

static void task1(void *unused)
{
    (void)unused;
    
    msgq_acquire(&task1q);
    
    bad_msg_block_t msg = {0};
    
    BAD_ASSERT(msgq_pull_msg(&task1q, &msg, 20) == BAD_RTOS_STATUS_OK, "Msg pull failed");
    
    BAD_ASSERT(msg.signal == MSGQ_SIGNAL && (u32)msg.args == MSGQ_ARG, "Msg data mismatch");
    
    msgq_release(&task1q);
    task_finish();
}

static void isr_test()
{
    msgq_post_msg_from_isr(&task1q,MSGQ_SIGNAL,(void *) MSGQ_ARG);
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,msgq_from_isr) = {
    .task1_descr = &task1_descr,
    .isr_test_func = isr_test,
    .num_testcases = 2,
    .test_name = "MSGQ from isr"
};

#endif