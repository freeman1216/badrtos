#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_EVENT_BARRIER

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

#define REPORTED_FLAGS 0x3

static bad_event_barrier_t evb;
static volatile u32 switched_to_second;

static void task1(void *unused)
{
    (void)unused;
    
    event_barrier_prime(&evb, 2);
    {
        u32 flags = event_barrier_wait(&evb, 0);
        
        u32 stripped = EVENT_BARRIER_GET_FLAGS(flags);
        BAD_ASSERT(switched_to_second,"Block check failed");
        BAD_ASSERT(stripped == REPORTED_FLAGS,"Flag check failed");
    }
    
    event_barrier_prime(&evb, 2);
    {
        u32 flags = event_barrier_wait(&evb, 2);
        
        BAD_ASSERT(EVENT_BARRIER_GET_ERROR(flags) == BAD_RTOS_STATUS_TIMEOUT,"Timeout check failed");
        
        task_unblock(task2h);
    }
    
    event_barrier_prime(&evb, 2);
    {
        u32 flags = event_barrier_wait(&evb, 0);
        BAD_ASSERT(EVENT_BARRIER_GET_ERROR(flags) == BAD_RTOS_STATUS_DELETED,"Delete check failed");
    }
    
    {
        u32 flags = event_barrier_wait(&evb, 0);
        BAD_ASSERT(EVENT_BARRIER_GET_ERROR(flags) == BAD_RTOS_STATUS_NOT_INITIALISED,"Noinit check failed");
    }
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    
    static u32 sent_flags = 0x1;
    
    switched_to_second = 1;
    
    for(u32 i = 0; i < 2; i++)
    {
        event_barrier_fire(&evb, sent_flags);
        sent_flags ^= 0x3;
    }
    
    task_block();
    
    event_barrier_delete(&evb);
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,evb_base) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 5,
    .test_name = "EVB base api"
};
#endif
