#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MUTEX

#define TASK1_PRIORITY 1

static MUTEX_DECLARE(mut);

static void task1(void *unused)
{
    (void)unused;
    
    for(u32 i = 0; i < 4; i++)
    {
        mutex_take(&mut,0);
    }
    
    for(u32 i = 0; i < 4; i++)
    {
        mutex_put(&mut);
    }
    
    BAD_ASSERT(!mut.owner && !mut.rec_takes, "Mutex recursive take/put release failed");
    
    task_finish();
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 50,
    .base_priority = TASK1_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,mutex_rec) = {
    .task1_descr = &task1_descr,
    .num_testcases = 1,
    .test_name = "Mutex recursive takes"
};

#endif