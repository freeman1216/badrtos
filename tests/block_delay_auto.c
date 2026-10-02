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

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,block_delay) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 3,
    .test_name = "Block delay"
};