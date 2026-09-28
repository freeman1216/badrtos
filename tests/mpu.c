#include "platform_include.h"
#include "runner.h"

#define TASK1_PRIORITY 1 

extern u8 __kernel_bss;

static volatile u32 seq;
static volatile u32 fault_count;

static void memfault_func()
{
    fault_count++;
}

static void task1(void *unused)
{ 
    (void)unused;
    u8 *kernel_bss_ptr = &__kernel_bss;
    u8 try_read = *kernel_bss_ptr;
    
    if(fault_count == 1)
        bad_test_check_in();
    else
        bad_test_fail();
    
    char nullptr_read = *(char *)0;
    
    if(fault_count == 2)
        bad_test_check_in();
    else
        bad_test_fail();
    
    u32 allowed_read = *BAD_PLATFORM_MPU_TEST_ADDR;
    
    if(fault_count == 3)
        bad_test_fail();
    else
        bad_test_check_in();
    
    task_finish();
}

static const bad_mpu_user_region_t task1r[] = {
    {
        .addr = (u8 *) BAD_PLATFORM_MPU_TEST_ADDR,
        .type = BAD_MPU_REGION_DEVICE_NGRE,
        .settings = BAD_MPU_PRIV_RW_UNPRIV_RW | BAD_MPU_EXECUTE_NEVER,
        .size = 32
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
    .task1_descr = &task1_descr,
    .memfault_func = memfault_func,
    .num_testcases = 3,
    .test_name = "MPU"
};
