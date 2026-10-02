#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 1

static SEM_DECLARE(sem,0);

static void task1(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(sem_take(&sem,20) == BAD_RTOS_STATUS_OK,"Post from isr failed");
    task_finish();
}

static void isr_test()
{
    sem_put_from_isr(&sem);
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 500,
    .base_priority = TASK1_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_from_isr) = {
    .task1_descr = &task1_descr,
    .isr_test_func = isr_test,
    .num_testcases = 1,
    .test_name = "Sem from isr"
};

#endif
