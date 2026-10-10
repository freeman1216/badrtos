#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

static volatile u32 success_count;

static void cb(bad_task_handle_t unused0, void* unused1)
{
    (void)unused0;
    (void)unused1;
    
    bad_test_fail(__FILE__,__LINE__,"Delay cb reached");
}

static void task1(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(task_delay(20, cb, 0) == BAD_RTOS_STATUS_WOKEN, "Task delay failed or wasn't woken");
    
    BAD_ASSERT(success_count >= 2, "Delay cancel from ISR returned wrong status");
    
    task_finish();
}


static void task2(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(task_delay(20, cb, 0) == BAD_RTOS_STATUS_WOKEN, "Task delay failed or wasn't woken");
    
    BAD_ASSERT(success_count >= 2, "Delay cancel from ISR returned wrong status");
    
    task_finish();
}

static void isr_test()
{
    success_count += task_delay_cancel_from_isr(task1h) == BAD_RTOS_STATUS_OK;
    success_count += task_delay_cancel_from_isr(task2h) == BAD_RTOS_STATUS_OK;
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,task_delay_cancel_isr) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .isr_test_func = isr_test,
    .num_testcases = 4,
    .test_name = "Task delay cancel isr"
};