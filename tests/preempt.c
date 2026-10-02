#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 2
#define TASK2_PRIORITY 1

static volatile u32 preempted;

static void task2(void *unused)
{
    (void)unused;
    
    preempted = 1;
    
    bad_test_check_in();
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    {
        bad_task_descr_t task2_descr = {
            .stack = task2_stack,
            .stack_size = TASK2_STACK_SIZE,
            .entry = task2,
            .ticks_to_change = 50,
            .base_priority = TASK2_PRIORITY
        };
        
        preempt_disable();
        {
            bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
            
            task2h = task_make(&task2_descr);
            
            ret = BAD_TASK_HANDLE_GET_ERROR(task2h);
            
            BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Task make failed");
            BAD_ASSERT(!preempted, "Preempted while preemption disabled");
        }
        preempt_enable();
        
        BAD_ASSERT(preempted, "Task not preempted after preempt_enable");
    }
    task_finish();
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 50,
    .base_priority = TASK1_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,preempt_lock) = {
    .task1_descr = &task1_descr,
    .num_testcases = 4,
    .test_name = "Preempt lock"
};