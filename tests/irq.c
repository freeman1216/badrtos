#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

#define OWNED_IRQN 0
#define BOGUS_IRQN 1

static void irq_test_ownership(u32 seq)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    ret = irq_acquire(OWNED_IRQN);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Acquire OWNED_IRQN failed");
    
    ret = irq_acquire(OWNED_IRQN);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_ALLOC_FAIL, "Acquire OWNED_IRQN twice alloc fail check failed");
    
    if(!seq)
        task_block();
    
    ret = irq_disable(OWNED_IRQN);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Disable OWNED_IRQN failed");
    
    ret = irq_disable(BOGUS_IRQN);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_NOT_OWNER, "Disable BOGUS_IRQN not owner check failed");
    
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    
    irq_test_ownership(0);
}

static void task2(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(irq_acquire(OWNED_IRQN) != BAD_RTOS_STATUS_OK, "Task2 acquire OWNED_IRQN should fail");
    
    task_unblock(task1h);
    
    irq_test_ownership(1);
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,irq_own) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 7,
    .test_name = "IRQ ownership"
};