#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

static SEM_DECLARE(sem,1);
static volatile u32 switched_to_second;
static volatile u32 seq;

static void task1(void *unused)
{
    (void)unused;
    task_block();
    {
        BAD_ASSERT(sem_take(&sem,1) == BAD_RTOS_STATUS_TIMEOUT,"Timeout check failed");
        BAD_ASSERT(switched_to_second,"Block check failed");
    }
    
    {
        task_unblock(task2h);
        BAD_ASSERT(sem_take(&sem,0) == BAD_RTOS_STATUS_DELETED,"Delete check failed");
    }
    
    {
        BAD_ASSERT(sem_take(&sem,-1) == BAD_RTOS_STATUS_NOT_INITIALISED,"Delete check failed");
    }
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    {
        BAD_ASSERT(sem_take(&sem, -1) == BAD_RTOS_STATUS_OK,"Init check failed");
        
        task_unblock(task1h);
    }
    
    {
        switched_to_second = 1;
        task_block();
    }
    
    {
        sem_delete(&sem);
    }
    
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

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_base) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 5,
    .test_name = "Sem base API"
};

#endif
