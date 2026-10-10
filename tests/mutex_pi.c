#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MUTEX

#define TASK1_PRIORITY 2 
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1

static MUTEX_DEFINE(mut);

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
        
        BAD_ASSERT(preempted == 1, "Task 1 completed mutex release");
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
        
        BAD_ASSERT(1, "Task 2 acquired and released mutex");
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
        
        BAD_ASSERT(preempted == 1, "Task 3 executed after priority inheritance");
    }
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);
TASK_DESCR(task3_descr,3,task3,TASK3_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,mutex_pi) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = 3,
    .test_name = "Mutex priority inheritance"
};

#endif