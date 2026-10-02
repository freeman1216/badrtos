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

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 500,
    .base_priority = TASK1_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,evb_from_isr) = {
    .task1_descr = &task1_descr,
    .isr_test_func = isr_test,
    .num_testcases = 1,
    .test_name = "EVB from isr"
};

#endif