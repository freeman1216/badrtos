#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_EVENT_BARRIER

#define TASK1_PRIORITY 1 

#define REPORTED_FLAGS 0x3

static bad_event_barrier_t evb;

static void task1(void *unused)
{
    (void)unused;
    
    event_barrier_prime(&evb, 2);
    u32 flags = event_barrier_wait(&evb, 0);
    
    u32 stripped = EVENT_BARRIER_GET_FLAGS(flags);
    
    BAD_ASSERT(stripped == REPORTED_FLAGS, "EVB flags mismatch");
    
    task_finish();
}

static void isr_test()
{
    static u32 sent_flags = 0x1;
    event_barrier_fire_from_isr(&evb, sent_flags);
    sent_flags ^= 0x3;
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,evb_from_isr) = {
    .task1_descr = &task1_descr,
    .isr_test_func = isr_test,
    .num_testcases = 1,
    .test_name = "EVB from isr"
};

#endif