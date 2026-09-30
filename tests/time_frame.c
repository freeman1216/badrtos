#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

volatile u32 seq;

static void task1(void *unused)
{
    (void)unused;
    
    bad_test_check_in();
    
    while(!seq)
    {
        
    }
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    
    bad_test_check_in();
    
    seq = 1;
    
    task_finish();
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 20,
    .base_priority = TASK1_PRIORITY
};

static const bad_task_descr_t task2_descr = {
    .stack = task2_stack,
    .stack_size = TASK2_STACK_SIZE,
    .entry = task2,
    .ticks_to_change = 20,
    .base_priority = TASK2_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,timeframe) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 2,
    .test_name = "Timeframe"
};
