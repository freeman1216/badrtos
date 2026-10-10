#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 2 
#define TASK2_PRIORITY 3
#define TASK3_PRIORITY 1

static SEM_DEFINE(sem,1);
static SEM_DEFINE(preempt_sem,0);

static volatile u32 switched_to_second;
static volatile u32 seq;
static volatile u32 task3_ran;

static void task1(void *unused)
{
    (void)unused;
    task_block();
    {
        BAD_ASSERT(sem_take(&sem,1) == BAD_RTOS_STATUS_TIMEOUT,"Timeout check failed");
        BAD_ASSERT(switched_to_second,"Block check failed");
    }
    
    preempt_disable();
    {
        BAD_ASSERT(sem_take(&preempt_sem,0) == BAD_RTOS_STATUS_SCHED_LOCKED,"Preempt check failed");
        BAD_ASSERT(sem_put(&preempt_sem) == BAD_RTOS_STATUS_OK,"Sem put with preempt lock failed");
        BAD_ASSERT(!task3_ran,"Task was preempteed with preempt lock");
    }
    preempt_enable();
    BAD_ASSERT(task3_ran,"Task was not preempteed without preempt lock");
    
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

static void task3(void *unused)
{
    (void)unused;
    
    sem_take(&preempt_sem,0);
    
    task3_ran = 1;
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);
TASK_DESCR(task3_descr,3,task3,TASK3_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_base) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = 9,
    .test_name = "Sem base API"
};

#endif
