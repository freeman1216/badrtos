#include "badrtos_split.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_KHEAP

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1

#define NUM_BLOCKS 8
#define POISON_BASE 0xDEADBEEF

static void execute_poison_test(u32 *sizes, u32 *free_order, u32 poison_offset)
{
    u32 *ptrs[NUM_BLOCKS] = {0};
    
    for(u32 i = 0; i < NUM_BLOCKS; i++)
    {
        ptrs[i] = kernel_alloc(sizes[i]);
        BAD_ASSERT(ptrs[i] != 0, "Kernel alloc failed");
        
        {
            u32 words = sizes[i] / sizeof(u32);
            for (u32 w = 0; w < words; w++)
            {
                ptrs[i][w] = POISON_BASE + poison_offset + i;
            }
        }
        
        task_yield();
    }
    
    for(u32 i = 0; i < NUM_BLOCKS; i++)
    {
        u32 words = sizes[i] / sizeof(u32);
        for (u32 w = 0; w < words; w++)
        {
            BAD_ASSERT(ptrs[i][w] == (POISON_BASE + poison_offset + i), "Poison data check failed");
        }
    }
    
    for(u32 i = 0; i < NUM_BLOCKS; i++)
    {
        u32 idx = free_order[i];
        kernel_free(ptrs[idx], sizes[idx]);
        task_yield();
    }
    
    task_finish();
}

static void task1(void *unused)
{
    (void)unused;
    
    u32 sizes[NUM_BLOCKS] = {16, 32, 16, 64, 128, 32, 256, 16};
    u32 free_order[NUM_BLOCKS] = {7, 0, 3, 5, 1, 6, 2, 4}; 
    
    execute_poison_test(sizes, free_order, 0x1000);
}

static void task2(void *unused)
{
    (void)unused;
    
    u32 sizes[NUM_BLOCKS] = {64, 16, 128, 32, 16, 256, 32, 64};
    u32 free_order[NUM_BLOCKS] = {1, 3, 5, 7, 0, 2, 4, 6}; 
    
    execute_poison_test(sizes, free_order, 0x2000);
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

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, buddy_poison_test) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = NUM_BLOCKS * 2 * 2,
    .test_name = "Buddy Poison & Order"
};

#endif