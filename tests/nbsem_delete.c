#define BAD_USART_IMPLEMENTATION
#define BAD_FLASH_IMPLEMENTATION
#define BAD_RCC_IMPLEMENTATION
#define BAD_GPIO_IMPLEMENTATION

#define BAD_HARDFAULT_USE_UART
#define BAD_HARDFAULT_ISR_IMPLEMENTATION

#define BAD_RTOS_IMPLEMENTATION
#include "badrtos.h"

#define UART_GPIO_PORT          (GPIOA)
#define UART1_TX_PIN            (9)
#define UART1_RX_PIN            (10)
#define UART1_TX_AF             (7)
#define UART1_RX_AF             (7)

    // HSE  = 25
    // PLLM = 25
    // PLLN = 400
    // PLLQ = 10
    // PLLP = 4
    // Sysclock = 100

#define BADHAL_PLLM (25)
#define BADHAL_PLLN (400)
#define BADHAL_PLLQ (10)
#define BADHAL_FLASH_LATENCY (FLASH_LATENCY_3ws)

#define BAD_RTOS_AHB1_PERIPEHRALS    (RCC_AHB1_GPIOA|RCC_AHB1_DMA2|RCC_AHB1_GPIOB)
#define BAD_RTOS_APB2_PERIPHERALS    (RCC_APB2_USART1|RCC_APB2_SPI1|RCC_APB2_SYSCFGEN|RCC_APB2_TIM10)


static inline void __main_clock_setup(){
    rcc_enable_hse();
    rcc_pll_setup( PLLP4, BADHAL_PLLM, BADHAL_PLLN, BADHAL_PLLQ, PLL_SOURCE_HSE);
    rcc_bus_prescalers_setup(HPRE_DIV_1, PPRE_DIV_2, PPRE_DIV_1);
    flash_acceleration_setup(BADHAL_FLASH_LATENCY, FLASH_DCACHE_ENABLE, FLASH_ICACHE_ENABLE);
    rcc_enable_and_switch_to_pll();
}


static inline void __periph_setup(){
    rcc_set_ahb1_clocking(BAD_RTOS_AHB1_PERIPEHRALS);
    io_setup_pin(UART_GPIO_PORT, UART1_TX_PIN, MODER_af, UART1_TX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    io_setup_pin(UART_GPIO_PORT, UART1_RX_PIN, MODER_af, UART1_RX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    rcc_set_apb2_clocking(BAD_RTOS_APB2_PERIPHERALS);
}

START_TASK_MPU_REGIONS_DEFINITIONS(task1)
    DEFINE_PERIPH_ACCESS_REGION(USART1_BASE, sizeof(USART_typedef_t))
END_TASK_MPU_REGIONS(task1)

bad_tcb_t* task1tcb;
bad_tcb_t* task2tcb;
bad_nbsem_t sem ;
void task1(){
    while (1) {
        nbsem_take(&sem);
        task_yield();
        nbsem_put(&sem);
        task_yield();
        while (1) {
            if(nbsem_delete(&sem) == BAD_RTOS_STATUS_OK){
                task_finish();
            }else{
                task_yield();
            }
        }
    }
}

void task2(){
    while (1) {
        bad_rtos_status_t status;
        status = nbsem_take(&sem);
        if(status != BAD_RTOS_STATUS_OK){
            task_finish();
        }
        task_yield();
        nbsem_put(&sem);
        task_yield();
    }
}
void task3(){
    while (1) {
        bad_rtos_status_t status;
        status = nbsem_take(&sem);
        if(status != BAD_RTOS_STATUS_OK){
            task_finish();
        }
        task_yield();
        nbsem_put(&sem);
        task_yield();
    }
}


#define TASK1_PRIORITY 1 
#define TASK2_PRIORITY 1
#define TASK3_PRIORITY 1
#define TASK1_STACK_SIZE 1024
#define TASK2_STACK_SIZE 1024
#define TASK3_STACK_SIZE 1024
TASK_STATIC_STACK(task2, TASK2_STACK_SIZE);
TASK_STATIC_STACK(task3, TASK2_STACK_SIZE);
void bad_user_setup(){
    bad_task_descr_t task1_descr = {
        .stack = 0,
        .stack_size = TASK1_STACK_SIZE,
        .dyn_stack = 1,
        .entry = task1,
        .regions = task1_regions,
        .region_count = MPU_REGIONS_SIZE(task1),
        .ticks_to_change = 500,
        .base_priority = TASK1_PRIORITY
    };
    task1tcb = task_make(&task1_descr);
    bad_task_descr_t task2_descr = {
        .stack = task2_stack,
        .stack_size = TASK2_STACK_SIZE,
        .entry = task2,
        .ticks_to_change = 500,
        .base_priority = TASK2_PRIORITY
    };
    task2tcb = task_make(&task2_descr);
    bad_task_descr_t task3_descr = {
        .stack = task3_stack,
        .stack_size = TASK3_STACK_SIZE,
        .entry = task3,
        .ticks_to_change = 500,
        .base_priority = TASK3_PRIORITY
    };
    task2tcb = task_make(&task3_descr);
    nbsem_init(&sem, 2);
}


int __attribute__((noinline)) main(){
    __DISABLE_INTERUPTS;
    __main_clock_setup();
    __periph_setup();
    __ENABLE_INTERUPTS;
    bad_rtos_start();
    //task_yield();
    while(1){

    
        
    }
    return 0;
}
