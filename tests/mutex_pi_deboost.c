#include "badrtos_split.h"
#include "runner.h"

#if defined(BAD_RTOS_USE_MUTEX) && !defined(BAD_RTOS_MUTEX_SIMPLE_PI) && defined (BAD_RTOS_USE_SEMAPHORE)

#define TASK1_PRIORITY 3
#define TASK2_PRIORITY 2
#define TASK3_PRIORITY 1
#define TASK4_PRIORITY 4

static MUTEX_DEFINE(mut);
static SEM_DEFINE(sem,0);
static volatile u32 task2_ran;
static volatile u32 seq;

static void task4(void *unused)
{
    (void)unused;
    
    while(!seq)
    {
        __asm__("wfi \n" :::"memory");
    }
    
    sem_put(&sem);
    
    while(seq == 1)
    {
        __asm__("wfi \n" :::"memory");
    }
    
    sem_put(&sem);
    
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    
    mutex_take(&mut, 0);
    
    sem_take(&sem,0);
    
    BAD_ASSERT(!task2_ran,"task2 preempted task1 while it was boosted");
    
    sem_take(&sem,0);
    
    BAD_ASSERT(task2_ran,"task2 did not preempted task1 while it wasnt boosted");
    
    mutex_put(&mut);
    
    task_finish();
}

static void task2(void *unused)
{ 
    (void)unused;
    
    sem_take(&sem,0);
    
    task2_ran = 1;
    
    sem_put(&sem);
    
    task_finish();
}

static void task3(void *unused)
{
    (void)unused;
    
    task_delay(5, 0, 0); // let task1 take mut and block on the semaphore
    
    seq = 1;
    BAD_ASSERT(mutex_take(&mut, 10) == BAD_RTOS_STATUS_TIMEOUT,"this should timeout");
    seq = 2;
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);
TASK_DESCR(task3_descr,3,task3,TASK3_PRIORITY);
TASK_DESCR(task4_descr,4,task4,TASK4_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, mutex_deboost_sem) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .task4_descr = &task4_descr,
    .num_testcases = 3,
    .test_name = "Mutex PI deboost"
};

#endif