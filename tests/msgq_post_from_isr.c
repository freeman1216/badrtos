#define BAD_RTOS_ISR_TEST
#define BAD_RTOS_PLATFORM_IMPLEMENTATION
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;

MSGQ_STATIC_INIT(task1q, 16);


void task1(){
    volatile uint32_t sig0 = 0;
    volatile uint32_t sig1 = 0;
    volatile uint32_t sig2 = 0;
    volatile uint32_t sig3 = 0;
    while (1) {
        bad_msg_block_t msg = {0};
        msgq_pull_msg(&task1q, &msg,0);
        switch (msg.signal) {
            case 0:{
                sig0++;
                break;
            }                
            case 1:{
                sig1++;
                break;
            }
            case 2:{
                sig2++;
                break;
            }
            case 3:{
                sig3++;
                break;
            }
            
        }
    }
}

uint32_t sig = 0;
void isr_test(){
    msgq_post_msg_from_isr(&task1q,sig,0);
    sig = (sig + 1) & 0x3;
}

#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK2_STACK_SIZE 1024
#define TASK1_STACK_SIZE 1024

TASK_STATIC_STACK(task1, TASK1_STACK_SIZE);



START_TASK_MPU_REGIONS_DEFINITIONS(task1)
#if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
DEFINE_STATIC_STACK_REGION(task1_stack,TASK2_STACK_SIZE)
#endif
END_TASK_MPU_REGIONS(task1)

void bad_user_setup(){
    bad_task_descr_t task1_descr = {
        .stack = task1_stack,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        .regions = task1_regions,
        .assigned_msgq = &task1q,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1h = task_make(&task1_descr);
}

int __attribute__((noinline)) main(){
    __platform_setup();
    bad_rtos_start();
    //task_yield();
    while(1){
        
        
        
    }
    return 0;
}
