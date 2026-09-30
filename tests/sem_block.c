#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

static SEM_DECLARE(sem,1);

static void task1(void *unused)
{
    (void)unused;
    
    sem_take(&sem,0);
    task_yield();
    
    if(sem.blockedq.next != &sem.blockedq)
    {
        sem_put(&sem);
        bad_test_check_in();
        task_finish();
    }
    else
    {
        bad_test_fail();
    }
}

static void task2(void *unused)
{
    (void)unused;
    
    sem_take(&sem,0);
    
    bad_test_check_in();
    task_finish();
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 500,
    .base_priority = TASK1_PRIORITY
};

static const bad_task_descr_t task2_descr = {
    .stack = task2_stack,
    .stack_size = TASK2_STACK_SIZE,
    .entry = task2,
    .ticks_to_change = 500,
    .base_priority = TASK2_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_block) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 2,
    .test_name = "Sem block"
};

#endif
