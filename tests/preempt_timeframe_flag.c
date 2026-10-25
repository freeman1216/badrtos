#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 1

static volatile u32 task2_ran;

static void task2(void *unused)
{
    (void)unused;
    
    task2_ran = 1;
    
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    {
        u32 ticks_b = get_ticks();
        preempt_disable();
        {
            while(get_ticks() - ticks_b <= 50);
            BAD_ASSERT(task2_ran == 0, "Task2 ran before preempt enable");
        }
        preempt_enable();
        
        BAD_ASSERT(task2_ran == 1, "Task2 didnt run after task1 timeframe ran out ");
    }
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,preempt_lock_timeframe) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 2,
    .test_name = "Preempt lock timeframe"
};