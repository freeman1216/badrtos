#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_FPU

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

static void task1(void *unused)
{
    (void)unused;
    
    float f = 2.0f;
    float fmul = 1.5f;
    float res = 0.0f;
    
    __asm__ volatile (
                      "vmul.f32 %0,%1,%2 \n"
                      "bl task_yield  \n"
                      : "=&t"(res)
                      : "t" (f), "t"(fmul)
                      : "lr", "memory"
                      );
    
    if(res == 3.0f)
    {
        bad_test_check_in();
        task_finish();
    }
    else
    {
        bad_test_fail();
    }
}

static void task2(void *unused)
{
    (void)unused;
    
    float f = 4.0f;
    float fmul = 1.5f;
    float res = 0.0f;
    
    __asm__ volatile (
                      "vmul.f32 %0,%1,%2 \n"
                      "bl task_yield  \n"
                      : "=&t"(res)
                      : "t" (f), "t"(fmul)
                      : "lr", "memory"
                      );
    
    if(res == 6.0f)
    {
        bad_test_check_in();
        task_finish();
    }
    else
    {
        bad_test_fail();
    }
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 500,
    .base_priority = TASK1_PRIORITY
};

static const bad_task_descr_t task2_descr = {
    .stack = task2_stack,
    .stack_size = TASK2_STACK_SIZE,
    .entry = task2,
    .ticks_to_change = 500,
    .base_priority = TASK2_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,fpu) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 2,
    .test_name = "FPU"
};

#endif
