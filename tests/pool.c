#include "badrtos_split.h"
#include "runner.h"

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 2

#define POOL_BLOCK_SIZE 16
#define POOL_NUM_BLOCKS 8
#define POOL_MEM_SIZE (POOL_BLOCK_SIZE * POOL_NUM_BLOCKS)
#define POOL_POISON1 (0x32232232)
#define POOL_POISON2 (0x13371337)

static u8 test_pool_mem[POOL_MEM_SIZE];
static bad_pool_t test_pool;

typedef struct
{
    u32 data[2];
} compile_check1_t;

POOL_DECLARE_TYPE_STATIC(comp_check,compile_check1_t,1);

static void task1(void *unused)
{
    (void)unused;
    
    BAD_ASSERT(pool_init(&test_pool, test_pool_mem, POOL_BLOCK_SIZE, POOL_MEM_SIZE) == BAD_RTOS_STATUS_OK, "Pool init failed"); //1
    
    u32 *ptrs[POOL_NUM_BLOCKS];
    
    for (u32 i = 0; i < POOL_NUM_BLOCKS; i++)
    {
        ptrs[i] = pool_alloc(&test_pool);
        BAD_ASSERT(ptrs[i] != 0, "Pool alloc failed"); //9
        
        ptrs[i][0] = POOL_POISON1 + i;
    }
    
    BAD_ASSERT(pool_alloc(&test_pool) == 0, "Pool over-alloc check failed");//10
    
    for (u32 i = 1; i < POOL_NUM_BLOCKS; i += 2)
    {
        pool_free(&test_pool, ptrs[i]);
    }
    
    task_block();
    
    for (u32 i = 0; i < POOL_NUM_BLOCKS; i += 2)
    {
        BAD_ASSERT(ptrs[i][0] == POOL_POISON1 + i, "Pool data corruption");//14
        
        pool_free(&test_pool, ptrs[i]);
    }
    
    task_finish();
}

static void task2(void *unused)
{
    (void)unused;
    
    u32 *ptrs[POOL_NUM_BLOCKS / 2];
    
    for (u32 i = 0; i < POOL_NUM_BLOCKS / 2; i++)
    {
        ptrs[i] = pool_alloc(&test_pool);
        BAD_ASSERT(ptrs[i] != 0, "Pool alloc failed");//18 
        
        ptrs[i][0] = POOL_POISON2 + i;
    }
    
    BAD_ASSERT(pool_alloc(&test_pool) == 0, "Pool over-alloc check failed");//19
    
    for (u32 i = 0; i < POOL_NUM_BLOCKS / 2; i++)
    {
        pool_free(&test_pool, ptrs[i]);
    }
    
    task_unblock(task1h);
    
    task_finish();
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

BAD_ITER_SECTION_MEMBER(tests, bad_test_case_t, pool_api_test) = {
    .task1_descr = &task1_descr,
    .task2_descr = &task2_descr,
    .num_testcases = 1 + POOL_NUM_BLOCKS + 1 + (POOL_NUM_BLOCKS / 2)
        + (POOL_NUM_BLOCKS / 2) + 1,
    .test_name = "Pool API Basic & Order"
};