#include "badrtos_split.h"
#include "platform_setup.h"
#include "runner.h"

#ifdef BAD_RTOS_USE_MPU

#define TASK1_PRIORITY 1 

extern u8 __kernel_bss;

static volatile u32 seq;
static volatile u32 fault_count;
static u8 *mpu_test_addr;

static bad_mpu_user_region_t task1r[];

static void memfault_func()
{
    fault_count++;
}

static void task1(void *unused)
{ 
    (void)unused;
    u8 *kernel_bss_ptr = &__kernel_bss;
    u8 try_read = VOLATILE_READ(*kernel_bss_ptr);
    
    BAD_ASSERT(fault_count == 1, "Kernel BSS read fault failed");
    
    char nullptr_read = VOLATILE_READ(*__platform_get_nullptr());
    
    BAD_ASSERT(fault_count == 2, "Nullptr read fault failed");
    
    u8 allowed_read = VOLATILE_READ(*mpu_test_addr);
    
    BAD_ASSERT(fault_count == 2, "Allowed read caused a fault");
    
    task_finish();
}

static void init_func()
{
    mpu_test_addr = task1r[0].addr = __platform_get_mpu_test_region_addr();
    task1r[0].size = __platform_get_mpu_test_region_size();
}

static bad_mpu_user_region_t task1r[] = {
    {
        .addr = 0,
        .type = BAD_MPU_REGION_DEVICE_NGRE,
        .settings = BAD_MPU_PRIV_RW_UNPRIV_RW | BAD_MPU_EXECUTE_NEVER,
        .size = 0
    },
    {0},
};

static const bad_task_descr_t task1_descr = {
    .stack = task1_stack,
    .stack_size = TASK1_STACK_SIZE,
    .entry = task1,
    .regions = task1r,
    .ticks_to_change = 20,
    .base_priority = TASK1_PRIORITY
};

BAD_ITER_SECTION_MEMBER(tests,bad_test_case_t,mpu_test) = {
    .init_func = init_func, 
    .task1_descr = &task1_descr,
    .memfault_func = memfault_func,
    .num_testcases = 3,
    .test_name = "MPU"
};

#endif