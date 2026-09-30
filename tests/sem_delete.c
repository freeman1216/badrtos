#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1

static SEM_DECLARE(sem,0);

static void task1(void *unused)
{
    (void)unused;
    
    task_yield();
    
    if(sem_delete(&sem) == BAD_RTOS_STATUS_OK)
    {
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
    
    bad_rtos_status_t status = sem_take(&sem,0);
    
    if(status == BAD_RTOS_STATUS_DELETED)
    {
        bad_test_check_in();
        task_finish();
    }
    else
    {
        bad_test_fail();
    }
}

static void task3(void *unused)
{
    (void)unused;
    
    bad_rtos_status_t status = sem_take(&sem,0);
    
    if(status == BAD_RTOS_STATUS_DELETED)
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

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_delete_t) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = 3,
    .test_name = "Sem delete"
};

#endif
