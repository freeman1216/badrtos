#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MSGQ

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

MSGQ_DECLARE_STATIC(q0,16);
MSGQ_DECLARE_STATIC(q1,16);

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
        ret = msgq_acquire_allocate(&dynamic,16);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Acquire allocate failed");
        
        ret = msgq_acquire_allocate(&dynamic,16);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_BAD_PARAMETERS, "Acquire allocate bad params check failed");
        
        ret = msgq_release_deallocate(&dynamic);
        BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Release deallocate failed");
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

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 50,
    .base_priority = TASK1_PRIORITY
};

static const bad_task_descr_t task2_descr = {
    .stack = task2_stack,
    .stack_size = TASK2_STACK_SIZE,
    .entry = task2,
    .ticks_to_change = 50,
    .base_priority = TASK2_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,msgq_own) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 14,
    .test_name = "MSGQ ownership"
};

#endif