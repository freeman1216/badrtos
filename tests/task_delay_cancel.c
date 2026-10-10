#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2

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
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(task_delay_cancel(task1h) == BAD_RTOS_STATUS_OK, "Task delay cancel failed");
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,task_delay_cancel_thread) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 2,
    .test_name = "Task delay cancel thread"
};