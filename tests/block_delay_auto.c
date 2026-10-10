#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

static void cb(bad_task_handle_t unused0, void* unused1)
{
    (void)unused0;
    (void)unused1;
    
    bad_test_check_in();
}

static void task1(void *unused)
{
    (void)unused;
    
    task_delay(20, cb, 0);
    task_block();
    
    bad_test_check_in();
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    
    while(task_unblock(task1h) != BAD_RTOS_STATUS_OK)
    {
        task_delay(20,0,0);
    }
    
    bad_test_check_in();
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,block_delay) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 3,
    .test_name = "Block delay"
};