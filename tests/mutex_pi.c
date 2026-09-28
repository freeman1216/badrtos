#include "platform_include.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MUTEX

#define TASK1_PRIORITY 2 
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1

static MUTEX_DECLARE(mut);

volatile u32 preempted;

static void task1(void *unused)
{
    (void)unused;
    {
        mutex_take(&mut,0);
        task_unblock(task2h);
        task_unblock(task3h);
        
        preempted = 1;
        mutex_put(&mut);
        
        bad_test_check_in();
    }
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    {
        task_block();
        
        mutex_take(&mut,0);
        mutex_put(&mut);
        
        bad_test_check_in();
    }
    task_finish();
}

static void task3(void *unused)
{
    (void)unused;
    {
        task_block();
        
        while(!preempted)
        {
            task_yield();
        }
        
        bad_test_check_in();
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

static const bad_task_descr_t task2_descr = {
    .stack = task2_stack,
    .stack_size = TASK2_STACK_SIZE,
    .entry = task2,
    .ticks_to_change = 50,
    .base_priority = TASK2_PRIORITY
};

static const bad_task_descr_t task3_descr = {
    .stack = task3_stack,
    .stack_size = TASK3_STACK_SIZE,
    .entry = task3,
    .ticks_to_change = 50,
    .base_priority = TASK3_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,mutex_pi) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = 3,
    .test_name = "Mutex priority inheritance"
};

#endif
