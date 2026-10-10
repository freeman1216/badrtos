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
    
    u32 ret = BAD_RTOS_STATUS_OK;
    
    __asm__ volatile(
                     "vldr.32 s16, %2 \n"
                     "vldr.32 s17, %3 \n"
                     "vmul.f32 s18, s16, s17 \n"
                     "mov r0,#0xFFFF  \n"
                     "bl task_yield   \n"
                     "mov %1, r0      \n"
                     "vstr.32 s18, %0 \n"
                     : "=m" (res), "=r"(ret)
                     : "m" (f), "m" (fmul)
                     : "r0","s16", "s17", "s18", "lr", "memory"
                     );
    
    BAD_ASSERT(res == 3.0f, "FPU calculation mismatch in task1");
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Wrong ret value");
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    
    float f = 4.0f;
    float fmul = 1.5f;
    float res = 0.0f;
    
    u32 ret = BAD_RTOS_STATUS_OK;
    
    __asm__ volatile(
                     "vldr.32 s16, %2 \n"
                     "vldr.32 s17, %3 \n"
                     "vmul.f32 s18, s16, s17 \n"
                     "mov r0,#0xFFFF  \n"
                     "bl task_yield   \n"
                     "mov %1, r0      \n"
                     "vstr.32 s18, %0 \n"
                     : "=m" (res), "=r"(ret)
                     : "m" (f), "m" (fmul)
                     : "r0","s16", "s17", "s18", "lr", "memory"
                     );
    
    BAD_ASSERT(res == 6.0f, "FPU calculation mismatch in task2");
    BAD_ASSERT(ret == BAD_RTOS_STATUS_OK, "Wrong ret value");
    
    task_finish();
}

TASK_DESCR(task1_descr,1,task1,TASK1_PRIORITY);
TASK_DESCR(task2_descr,2,task2,TASK2_PRIORITY);

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,fpu) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 2,
    .test_name = "FPU"
};

#endif