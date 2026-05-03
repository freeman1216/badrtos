#pragma once
void __platform_setup();


#ifdef BAD_RTOS_PLATFORM_IMPLEMENTATION

#define BAD_USART_IMPLEMENTATION
#define BAD_FLASH_IMPLEMENTATION
#define BAD_RCC_IMPLEMENTATION
#define BAD_GPIO_IMPLEMENTATION

#ifdef BAD_RTOS_ISR_TEST
#define BAD_TIMER_IMPLEMENTATION

#define BTIMER_TIM1_UP_TIM10_ISR_IMPLEMENTATION
#define BTIMER_USE_TIM10_USR
#endif

#define BAD_HARDFAULT_USE_UART
#define BAD_HARDFAULT_ISR_IMPLEMENTATION

#include "badhal_f411.h"

#define UART_GPIO_PORT          (GPIOA)
#define UART1_TX_PIN            (9)
#define UART1_RX_PIN            (10)
#define UART1_TX_AF             (7)
#define UART1_RX_AF             (7)

#define BADHAL_FLASH_LATENCY (FLASH_LATENCY_3ws)

#define BAD_RTOS_AHB1_PERIPEHRALS    (RCC_AHB1_GPIOA)
#define BAD_RTOS_APB2_PERIPHERALS    (RCC_APB2_USART1|RCC_APB2_TIM10)

#ifdef BAD_RTOS_ISR_TEST
#define BAD_BTIMER_TEST_ARR    (65535)
#define BAD_BTIMER_TEST_PSC    (1525)
#define BAD_BTIMER_TEST_INTR   (BTIMER_UPDATE)
#endif

static inline void __main_clock_setup(){
    flash_acceleration_setup(BADHAL_FLASH_LATENCY, FLASH_DCACHE_ENABLE, FLASH_ICACHE_ENABLE);
    rcc_sysclock_setup();
}


static inline void __periph_setup(){
    rcc_set_ahb1_clocking(BAD_RTOS_AHB1_PERIPEHRALS);
    io_setup_pin(UART_GPIO_PORT, UART1_TX_PIN, MODER_af, UART1_TX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    io_setup_pin(UART_GPIO_PORT, UART1_RX_PIN, MODER_af, UART1_RX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    rcc_set_apb2_clocking(BAD_RTOS_APB2_PERIPHERALS);
}

static inline void __tick_setup(){
    scb_set_core_interrupt_priority(SCB_SYSTICK_INTR,SCB_PRIO15);
    systick_setup(CLOCK_SPEED/1000, SYSTICK_FEATURE_CLOCK_SOURCE|SYSTICK_FEATURE_TICK_INTERRUPT);
    systick_enable();
}

#ifdef BAD_RTOS_ISR_TEST
static inline void __timer_setup(){
    basic_timer_setup(BTIM10, BAD_BTIMER_TEST_ARR, BAD_BTIMER_TEST_PSC, BAD_BTIMER_TEST_INTR);
    nvic_set_interrupt_priority(NVIC_TIM1_UP_TIM10_INTR,NVIC_PRIO14);
    tim_enable(BTIM10);
    nvic_clear_interrupt(NVIC_TIM1_UP_TIM10_INTR);
    nvic_enable_interrupt(NVIC_TIM1_UP_TIM10_INTR);
    dbgmcu_freeze_apb2_periphals(DBGMCU, DBGMCU_APB2_TIM10);
}

void isr_test();

void tim10_usr(){
    isr_test();
}
#endif


void __platform_setup() {
    __main_clock_setup();
    __periph_setup();
    __tick_setup();
#ifdef BAD_RTOS_ISR_TEST
    __timer_setup();
#endif
}
#endif
