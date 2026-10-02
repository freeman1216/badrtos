#define BAD_RTOS_IMPLEMENTATION
#include "badrtos_split.h"

#include "platform_setup.h"
#include "runner.h"

#define MEMMANAGE_IRQN (-16)

BAD_ITER_SECTION_EXTERN(tests,bad_test_case_t);

bad_task_handle_t task1h;
TASK_STATIC_STACK(task1,TASK1_STACK_SIZE);

bad_task_handle_t task2h;
TASK_STATIC_STACK(task2,TASK2_STACK_SIZE);

bad_task_handle_t task3h;
TASK_STATIC_STACK(task3,TASK3_STACK_SIZE);

bad_task_handle_t runnerh;
#define RUNNER_PRIORITY 0
#define RUNNER_STACK_SIZE 1024
TASK_STATIC_STACK(runner_task,RUNNER_STACK_SIZE);

bad_task_handle_t unblockerh;
#define UNBLOCKER_PRIORITY (BAD_RTOS_MAX_TASKS - 2)
#define UNBLOCKER_STACK_SIZE 1024
TASK_STATIC_STACK(unblocker_task,UNBLOCKER_STACK_SIZE);

const bad_test_case_t * volatile current_testcase = 0;

SEM_DECLARE(unblock_sem,0);

volatile u32 checkins_left;

void bad_test_init(u32 checkins_count)
{
    checkins_left = checkins_count;
}

void bad_test_check_in()
{
    u32 prev;
    do
    {
        prev = __ldrex(&checkins_left);
    }
    while(__strex(prev - 1, &checkins_left));
    
    if(prev == 1)
    {
        if(in_isr())
            sem_put_from_isr(&unblock_sem);
        else
            sem_put(&unblock_sem);
    }
}

void bad_test_fail(char *file, int line, char *test)
{
    (void)file;
    (void)line;
    (void)test;
    
    while(1)
    {
        
    }
}

void isr_test()
{
    if(current_testcase->isr_test_func)
        current_testcase->isr_test_func();
    else
        bad_test_fail(__FILE__,__LINE__,"Isr fired without testcase having a test func");
}

void timeout_handler()
{
    while(1)
    {
        
    }
}

__attribute__((naked))
void memmanage_isr(void)
{
    __asm__ volatile (
                      "tst lr, #4          \n" 
                      "ite eq              \n"
                      "mrseq r0, msp       \n"
                      "mrsne r0, psp       \n"
                      "b memmanage_c       \n"
                      );
}

__attribute__((used))
void memmanage_c(uint32_t *stack_frame) {
    
    if(current_testcase->memfault_func)
    {
        current_testcase->memfault_func();
        
        uint16_t *faulting_pc = (uint16_t *)stack_frame[6];
        
        uint16_t op = *faulting_pc;
        if((op & 0xF000) == 0xF000 || (op & 0xF800) == 0xE800) 
        {
            stack_frame[6] += 4;
        }
        else 
        {
            stack_frame[6] += 2; 
        }
    }
    else
    {
        bad_test_fail(__FILE__,__LINE__,"Memfault fired without testcase having a test func");
    }
}

void runner_task(void *unused)
{
    (void)unused;
    
    u32 periodic_irqn = __platform_get_periodic_irqn();
    
    if(irq_acquire(periodic_irqn) != BAD_RTOS_STATUS_OK)
        bad_test_fail(__FILE__,__LINE__,"Failed to acquire periodic irq");
    
    if(irq_acquire(MEMMANAGE_IRQN) != BAD_RTOS_STATUS_OK)
        bad_test_fail(__FILE__,__LINE__,"Failed to acquire memfault irq");
    
    BAD_ITER_SECTION_ITER_ALL(tests,bad_test_case_t,current_testcase)
    {
        if(current_testcase->init_func)
            current_testcase->init_func();
        if(current_testcase->task1_descr) 
            task1h = task_make(current_testcase->task1_descr);
        if(current_testcase->task2_descr)
            task2h = task_make(current_testcase->task2_descr);
        if(current_testcase->task3_descr)
            task3h = task_make(current_testcase->task3_descr);
        if(current_testcase->isr_test_func)
        {
            irq_clear(periodic_irqn);
            irq_enable(periodic_irqn);
        }
        if(current_testcase->memfault_func)
        {
            irq_clear(MEMMANAGE_IRQN);
            irq_enable(MEMMANAGE_IRQN);
        }
        
        if(BAD_TASK_HANDLE_GET_ERROR(task1h) != BAD_RTOS_STATUS_OK ||
           BAD_TASK_HANDLE_GET_ERROR(task2h) != BAD_RTOS_STATUS_OK ||
           BAD_TASK_HANDLE_GET_ERROR(task3h) != BAD_RTOS_STATUS_OK )
        {
            bad_test_fail(__FILE__,__LINE__,"Task creation failed");
        }
        
        bad_test_init(current_testcase->num_testcases);
        
        __platform_start_timeout();
        task_block();
        __platform_pause_timeout();
        
        if(current_testcase->isr_test_func)
        {
            irq_clear(periodic_irqn);
            irq_disable(periodic_irqn);
        }
        if(current_testcase->memfault_func)
        {
            irq_clear(MEMMANAGE_IRQN);
            irq_disable(MEMMANAGE_IRQN);
        }
    }
    
    current_testcase = 0;
    sem_put(&unblock_sem);
    
    task_finish();
}

void unblocker_task(void *unused)
{
    (void)unused;
    
    do
    {
        sem_take(&unblock_sem,0);
        task_unblock(runnerh);
    }
    while(current_testcase);
    
    task_finish();
}

bad_rtos_status_t bad_user_init()
{
    const bad_mpu_user_region_t runner_regions[] = 
    {
        {
            .addr = __platform_get_timeout_timer_addr(),
            .size = __platform_get_timeout_timer_size(),
            .type = BAD_MPU_REGION_DEVICE_NGRE,
            .settings = BAD_MPU_PRIV_RW_UNPRIV_RW | BAD_MPU_EXECUTE_NEVER,
        },
        {0}
    };
    
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    bad_task_descr_t runner_descr = {
        .stack = runner_task_stack,
        .stack_size = RUNNER_STACK_SIZE,
        .entry = runner_task,
        .regions = runner_regions,
        .ticks_to_change = UINT32_MAX,
        .base_priority = RUNNER_PRIORITY,
    };
    
    runnerh = task_make(&runner_descr);
    
    ret = BAD_TASK_HANDLE_GET_ERROR(runnerh);
    if(ret != BAD_RTOS_STATUS_OK)
        return ret;
    
    bad_task_descr_t unblocker_descr = {
        .stack = unblocker_task_stack,
        .stack_size = UNBLOCKER_STACK_SIZE,
        .entry = unblocker_task,
        .ticks_to_change = UINT32_MAX,
        .base_priority = UNBLOCKER_PRIORITY,
    };
    
    task2h = task_make(&unblocker_descr);
    
    ret = BAD_TASK_HANDLE_GET_ERROR(task2h);
    
    sem_init(&unblock_sem,0);
    
    return ret;
}

int __attribute__((noinline)) main()
{
    __platform_base_setup();
    __platform_timeout_setup(timeout_handler);
    __platform_periodic_irq_setup(isr_test);
    
    bad_rtos_start();
    
    bad_test_fail(__FILE__,__LINE__,"Badrtos start failed");
    
    while(1)
    {
        
    }
    
    return 0;
}
