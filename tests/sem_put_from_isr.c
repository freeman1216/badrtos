#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_SEMAPHORE

#define TASK1_PRIORITY 1

static SEM_DEFINE(sem,0);

static void task1(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(sem_take(&sem,20) == BAD_RTOS_STATUS_OK,"Post from isr failed");
    task_finish();
}

static void isr_test()
{
    sem_put_from_isr(&sem);
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,sem_from_isr) = {
    .task1_descr = &task1_descr,
    .isr_test_func = isr_test,
    .num_testcases = 1,
    .test_name = "Sem from isr"
};

#endif
