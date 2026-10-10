#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MUTEX

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

static MUTEX_DEFINE(mutex);
static volatile u32 switched_to_second;
static volatile u32 seq;

static void task1(void *unused)
{
    (void)unused;
    task_block();
    {
        BAD_ASSERT(mutex_take(&mutex,1) == BAD_RTOS_STATUS_TIMEOUT,"Timeout check failed");
        BAD_ASSERT(switched_to_second,"Block check failed");
    }
    
    {
        BAD_ASSERT(mutex_delete(&mutex) == BAD_RTOS_STATUS_NOT_OWNER,"Delete not owner check failed");
        task_unblock(task2h);
        BAD_ASSERT(mutex_take(&mutex,0) == BAD_RTOS_STATUS_DELETED,"Delete check failed");
    }
    
    {
        BAD_ASSERT(mutex_take(&mutex,-1) == BAD_RTOS_STATUS_NOT_INITIALISED,"Delete check failed");
    }
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    {
        BAD_ASSERT(mutex_take(&mutex, -1) == BAD_RTOS_STATUS_OK,"Init check failed");
        
        task_unblock(task1h);
    }
    
    {
        switched_to_second = 1;
        task_block();
    }
    
    {
        mutex_delete(&mutex);
    }
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,mutex_base) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 6,
    .test_name = "Mutex base API"
};

#endif
