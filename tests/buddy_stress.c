#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_KHEAP

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1

#define STRESS_ITERATIONS 50

static void stress_allocator(u32 size_a, u32 size_b)
{
    for(int i = 0; i < STRESS_ITERATIONS; i++)
    {
        u32 *ptr_a = kernel_alloc(size_a);
        u32 *ptr_b = kernel_alloc(size_b);
        
        BAD_ASSERT(ptr_a && ptr_b, "Stress alloc failed");
        
        task_yield();
        
        if(ptr_a)
            kernel_free(ptr_a, size_a);
        if(ptr_b)
            kernel_free(ptr_b, size_b);
    }
    
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    stress_allocator(16, 32);
}

static void task2(void *unused)
{
    (void)unused;
    stress_allocator(64, 16);
}

static void task3(void *unused)
{
    (void)unused;
    stress_allocator(32, 128);
}

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .ticks_to_change = 50,
    .base_priority = TASK1_PRIORITY
};

static const bad_task_descr_t task2_descr = {
    .stack = task2_stack,
    .stack_size = TASK2_STACK_SIZE,
    .entry = task2,
    .ticks_to_change = 50,
    .base_priority = TASK2_PRIORITY
};

static const bad_task_descr_t task3_descr = {
    .stack = task3_stack,
    .stack_size = TASK3_STACK_SIZE,
    .entry = task3,
    .ticks_to_change = 50,
    .base_priority = TASK3_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, buddy_stress_test) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .task3_descr = &task3_descr,
    .num_testcases = STRESS_ITERATIONS * 3,
    .test_name = "Buddy Allocator Stress"
};

#endif