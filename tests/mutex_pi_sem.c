#include "badrtos_split.h"
#include "runner.h"

#if defined(BAD_RTOS_USE_MUTEX) && !defined(BAD_RTOS_MUTEX_SIMPLE_PI) && defined (BAD_RTOS_USE_SEMAPHORE)

static MUTEX_DEFINE(mut);
static SEM_DEFINE(sem,0);
static volatile u32 task3_ran;

static void task1(void* unused)
{
    (void)unused;
    
    task_block();
    
    bad_rtos_status_t ret = mutex_take(&mut, 0);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "High task acquired A held by a semaphore-blocked owner");
    
    mutex_put(&mut);
    
    task_finish();
}

static void task2(void *unused)
{ 
    (void)unused;
    
    task_delay(5, 0, 0); // let task3 take mut and block on the semaphore
    
    task_unblock(task1h); // task1 blocks on mut, task3 is boosted to 1
    
    sem_put(&sem); // wakes task3, which must preempt this task right now
    BAD_ASSERT(task3_ran == 1, "Owner blocked on semaphore was boosted and ran before poster");
    
    task_finish();
}

static void task3(void *unused)
{
    (void)unused;
    
    mutex_take(&mut, 0);
    
    bad_rtos_status_t ret = sem_take(&sem, 0);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Owner woke from semaphore with OK");
    
    task3_ran = 1;
    
    mutex_put(&mut);
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,1);
TASK_DESCR(task2_descr,2,task2,2);
TASK_DESCR(task3_descr,3,task3,3);

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, mutex_pi_sem) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = 3,
    .test_name = "Mutex PI chain ends at semaphore"
};

#endif