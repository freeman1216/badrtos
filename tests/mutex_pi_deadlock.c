#include "badrtos_split.h"
#include "runner.h"

#if defined(BAD_RTOS_USE_MUTEX) && !defined(BAD_RTOS_MUTEX_SIMPLE_PI)

static MUTEX_DEFINE(a);
static MUTEX_DEFINE(b);
static MUTEX_DEFINE(c);

static void task1(void *unused)
{
    (void)unused;
    mutex_take(&a, 0);
    
    task_block();
    
    bad_rtos_status_t ret = mutex_take(&b, 0);     // blocks on task2
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Task 1 acquired B once the cycle was broken");
    
    mutex_put(&b);
    mutex_put(&a);
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    mutex_take(&b, 0);
    
    task_block();
    
    bad_rtos_status_t ret = mutex_take(&c, 0);     // blocks on task3
    
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Task 2 acquired C once the cycle was broken");
    
    mutex_put(&c);
    mutex_put(&b);
    
    task_finish();
}

static void task3(void *unused)
{
    (void)unused;
    
    mutex_take(&c, 0);
    
    task_unblock(task1h); // task1 blocks on B, task2 inherits 1
    
    task_unblock(task2h); // task2 blocks on C, task3 inherits 1
    bad_rtos_status_t ret = mutex_take(&a, 0); // closes A -> t1 -> B -> t2 -> C -> t3
    BAD_ASSERT(ret == BAD_RTOS_STATUS_DEADLOCK, "Cycle through three mutexes reported as deadlock");
    mutex_put(&c); // C -> task2 -> B -> task1, each finishes in turn
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,1);
TASK_DESCR(task2_descr,2,task2,2);
TASK_DESCR(task3_descr,3,task3,3);

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, mutex_pi_deadlock) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = 3,
    .test_name = "Mutex PI deadlock detection"
};

#endif