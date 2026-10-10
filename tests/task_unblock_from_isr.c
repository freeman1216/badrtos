#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1

static volatile u32 success_count;

static void task1(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(task_block() == BAD_RTOS_STATUS_WOKEN, "Task block failed or wasn't woken");
    
    BAD_ASSERT(success_count, "Unblock from ISR returned wrong status");
    
    task_finish();
}

static void isr_test()
{
    success_count += task_unblock_from_isr(task1h) == BAD_RTOS_STATUS_OK;
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,task_unblock_isr) = {
    .task1_descr = &task1_descr,
    .isr_test_func = isr_test,
    .num_testcases = 2,
    .test_name = "Task unblock isr"
};