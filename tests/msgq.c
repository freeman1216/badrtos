#include "platform_setup.h"
#define BAD_RTOS_IMPLEMENTATION
#include "platform_include.h"

bad_task_handle_t task1h;
bad_task_handle_t task2h;

MSGQ_STATIC_INIT(task1q, 16);

volatile uint32_t sig0;
volatile uint32_t sig1;
volatile uint32_t sig2;
volatile uint32_t sig3;

void task1(){
    while (1) {
        bad_msg_block_t msg = {0};
        while(msgq_pull_msg(&task1q, &msg) == BAD_QUEUE_OK){
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
        task_yield();
    }
}

void task2(){
    while (1) {
        uint32_t sig = 0;
        while(msgq_post_msg(&task1q, sig, 0) != BAD_QUEUE_OVERRUN){
            sig = (sig + 1) & 0x3; 
        }
    }
}


#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK2_STACK_SIZE 1024
#define TASK1_STACK_SIZE 1024
TASK_STATIC_STACK(task2, TASK2_STACK_SIZE);

START_TASK_MPU_REGIONS_DEFINITIONS(task2)
#if defined(BAD_PLATFORM_H562) || defined(BAD_PLATFORM_H562T)
    DEFINE_STATIC_STACK_REGION(task2_stack,TASK2_STACK_SIZE)
#endif
END_TASK_MPU_REGIONS(task2)

void bad_user_setup(){
    bad_task_descr_t task1_descr = {
        .stack = 0,
        .stack_size = TASK1_STACK_SIZE,
        .entry = task1,
        //.regions = task1_regions,
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1h = task_make(&task1_descr);
    bad_task_descr_t task2_descr = {
        .stack = task2_stack,
        .stack_size = TASK2_STACK_SIZE,
        .entry = task2,
        .regions = task1_regions,
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    task2h = task_make(&task2_descr);
}


int __attribute__((noinline)) main(){
    __DISABLE_INTERUPTS;
    __platform_setup();
    __ENABLE_INTERUPTS;
    bad_rtos_start();
    //task_yield();
    while(1){

    
        
    }
    return 0;
}
