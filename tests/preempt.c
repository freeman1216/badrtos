#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 2
#define TASK2_PRIORITY 1

static volatile u32 preempted;

static void task2(void *unused)
{
    (void)unused;
    
    preempted++;
    
    task_block();
    
    preempted++;
    
    bad_test_check_in();
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    {
        bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
        
        bad_task_descr_t task2_descr = {
            .stack = task2_stack,
            .stack_size = TASK2_STACK_SIZE,
            .entry = task2,
            .ticks_to_change = 50,
            .base_priority = TASK2_PRIORITY
        };
        
        preempt_disable();
        {
            task2h = task_make(&task2_descr);
            
            ret = BAD_TASK_HANDLE_GET_ERROR(task2h);
            
            BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Task make failed");
            BAD_ASSERT(!preempted, "Preempted while preemption disabled");
        }
        preempt_enable();
        BAD_ASSERT(preempted == 1, "Task not preempted after preempt_enable");
        
        preempt_disable();
        {
            ret = task_unblock(task2h);
            
            BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Task unblock failed");
            BAD_ASSERT(preempted == 1, "Preempted while preemption disabled");
            
            ret = task_yield();
            BAD_ASSERT(ret == BAD_RTOS_STATUS_SCHED_LOCKED, "Yielded with preempt lock");
        }
        preempt_enable();
        BAD_ASSERT(preempted == 2, "Task not preempted after preempt_enable");
        
    }
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,preempt_lock) = {
    .task1_descr = &task1_descr,
    .num_testcases = 8,
    .test_name = "Preempt lock"
};