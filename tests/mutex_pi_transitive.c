#include "badrtos_split.h"
#include "runner.h"

#if defined(BAD_RTOS_USE_MUTEX) && !defined(BAD_RTOS_MUTEX_SIMPLE_PI)

static MUTEX_DEFINE(a);
static MUTEX_DEFINE(b);

#define TASK1_PRIORITY 4
#define TASK2_PRIORITY 3
#define TASK3_PRIORITY 1
#define TASK4_PRIORITY 2

static volatile u32 task4_ran;

static void task1(void *unused)
{
    (void)unused;
    mutex_take(&b, 0); // L owns B
    
    task_unblock(task2h); // task2: takes A, blocks on B-> task1 inherits task2's prio
    task_unblock(task3h); // task3: blocks on A->task2, then task1 inherit task3's prio
    task_unblock(task4h); // task4 is above task1's base, but must not be above task1 now
    
    BAD_ASSERT(!task4_ran, "Owner inherited priority transitively (bystander did not preempt)");
    
    mutex_put(&b); // B -> task2, task1 drops to base, task2/task3/task4 run to completion
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    task_block();
    mutex_take(&a, 0);
    bad_rtos_status_t ret = mutex_take(&b, 0);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Middle task acquired B after owner released it");
    mutex_put(&b);
    mutex_put(&a);
    task_finish();
}

static void task3(void *unused)
{
    (void)unused;
    task_block();
    bad_rtos_status_t ret = mutex_take(&a, 0);
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "High task acquired A after chain unwound");
    mutex_put(&a);
    task_finish();
}

static void task4(void *unused)
{
    (void)unused;
    task_block();
    task4_ran = 1;
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);
TASK_DESCR(task3_descr,3,task3,TASK3_PRIORITY);
TASK_DESCR(task4_descr,4,task4,TASK4_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, mutex_pi_chain) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .task4_descr = &task4_descr,
    .num_testcases = 3,
    .test_name = "Mutex PI transitive chain"
};

#endif