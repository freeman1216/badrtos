#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MUTEX

#define TASK1_PRIORITY 1

static MUTEX_DEFINE(mut);

static void task1(void *unused)
{
    (void)unused;
    
    for(u32 i = 0; i < 4; i++)
    {
        mutex_take(&mut,0);
    }
    
    for(u32 i = 0; i < 4; i++)
    {
        mutex_put(&mut);
    }
    
    BAD_ASSERT(!mut.owner && !mut.rec_takes, "Mutex recursive take/put release failed");
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,mutex_rec) = {
    .task1_descr = &task1_descr,
    .num_testcases = 1,
    .test_name = "Mutex recursive takes"
};

#endif