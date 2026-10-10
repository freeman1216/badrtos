#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MSGQ

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

MSGQ_DEFINE_STATIC(q0,16);
MSGQ_DEFINE_STATIC(q1,16);

static void msgq_test_ownership(u32 seq)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    bad_msg_block_t msg = {0};
    bad_msgq_t dynamic = {0};
    
    {
        ret = msgq_acquire(&q0);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Acquire q0 failed");
    }
    
    if(!seq)
        task_block();
    
    {
        ret = msgq_pull_msg(&q0,&msg,-1);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_WOULD_BLOCK, "Pull q0 would block failed");
    }
    
    {
        ret = msgq_pull_msg(&q1,&msg,-1);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_NOT_OWNER, "Pull q1 not owner failed");
    }
    
    {
        ret = msgq_release(&q1);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_NOT_OWNER, "Release q1 not owner failed");
    }
    
    {
        ret = msgq_release(&q0);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Release q0 failed");
    }
    
    if(!seq)
    {
#ifdef BAD_RTOS_USE_KHEAP
        ret = msgq_acquire_allocate(&dynamic,16);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Acquire allocate failed");
        
        ret = msgq_acquire_allocate(&dynamic,16);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_BAD_PARAMETERS, "Acquire allocate bad params check failed");
        
        ret = msgq_release_deallocate(&dynamic);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Release deallocate failed");
#endif
    }
    
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    
    msgq_test_ownership(0);
}

static void task2(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(msgq_acquire(&q0) != BAD_RTOS_STATUS_OK, "Task2 acquire q0 should fail");
    
    task_unblock(task1h);
    
    msgq_test_ownership(1);
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,msgq_own) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
#ifdef BAD_RTOS_USE_KHEAP
    .num_testcases = 14,
#else
    .num_testcases = 11,
#endif
    .test_name = "MSGQ ownership"
};

#endif