#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 1

static SEM_DECLARE(sem,0);

static void task1(void *unused)
{
    (void)unused;
    
    bad_rtos_status_t res = sem_take(&sem,100);
    
    if(res == BAD_RTOS_STATUS_TIMEOUT)
    {
        bad_test_check_in();
        task_finish();
    }
    else
    {
        bad_test_fail();
    }
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 500,
    .base_priority = TASK1_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_delay) = {
    .task1_descr = &task1_descr,
    .num_testcases = 1,
    .test_name = "Sem delay"
};

#endif
