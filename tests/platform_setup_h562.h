#pragma once
void __platform_setup();

#ifdef BAD_RTOS_PLATFORM_IMPLEMENTATION

#define BAD_USART_IMPLEMENTATION
#define BAD_RCC_IMPLEMENTATION
#define BAD_FLASH_IMPLEMENTATION
#define BAD_GPIO_IMPLEMENTATION
#define BAD_PWR_IMPLEMENTATION
#define BAD_BTIMER_IMPLEMENTATION

#ifdef BAD_RTOS_ISR_TEST
#  define BTIMER_TIM6_ISR_IMPLEMENTATION
#endif

#define BAD_HARDFAULT_ISR_IMPLEMENTATION
#define BAD_HARDFAULT_USE_UART

#include "badhal_h562.h"

#define UART_GPIO_PORT          (GPIOA)
#define UART1_TX_PIN            (9)
#define UART1_RX_PIN            (10)
#define UART1_TX_AF             (7)
#define UART1_RX_AF             (7)

#ifdef BAD_RTOS_ISR_TEST
#define BAD_BTIMER_TEST_ARR    (65535)
#define BAD_BTIMER_TEST_PSC    (1525)
#define BAD_BTIMER_TEST_INTR   (BTIMER_UPDATE)
#endif

#define BAD_RTOS_FLASH_LATENCY    (FLASH_LATENCY_5ws)

#define BAD_RTOS_APB1L_PERIPHERALS (RCC_APB1L_TIM6)

#define BAD_RTOS_AHB2_PERIPEHRALS    (RCC_AHB2_GPIOA|RCC_AHB2_GPIOC|RCC_AHB2_SRAM3|RCC_AHB2_SRAM2)
#define BAD_RTOS_APB2_PERIPHERALS    (RCC_APB2_USART1)

#define BAD_RTOS_SETTINGS (USART_FEATURE_RECIEVE_EN|USART_FEATURE_TRANSMIT_EN)

static inline void __main_clock_setup(){
    flash_acceleration_setup(FLASH_REGS,BAD_RTOS_FLASH_LATENCY, FLASH_PROGRAMMING_DELAY_2);
    pwr_setup_vos(PWR,PWR_VOS0);
    rcc_default_sysclock_setup(RCC);
}

static inline void __periph_setup(){
    rcc_set_ahb2_clocking(RCC,BAD_RTOS_AHB2_PERIPEHRALS);
    io_setup_pin(UART_GPIO_PORT, UART1_TX_PIN, MODER_af, UART1_TX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    io_setup_pin(UART_GPIO_PORT, UART1_RX_PIN, MODER_af, UART1_RX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    //Enable UART clocking
    rcc_set_apb2_clocking(RCC,BAD_RTOS_APB2_PERIPHERALS);
    rcc_set_apb1l_clocking(RCC,BAD_RTOS_APB1L_PERIPHERALS);
}

static inline void __tick_setup(){
    scb_set_core_interrupt_priority(SCB_SYSTICK_INTR,SCB_PRIO15);
    systick_setup(CLOCK_SPEED/1000, SYSTICK_FEATURE_CLOCK_SOURCE|SYSTICK_FEATURE_TICK_INTERRUPT);
    systick_enable();
}

#ifdef BAD_RTOS_ISR_TEST
static inline void __timer_setup(){
    basic_timer_setup(BTIM6, BAD_BTIMER_TEST_ARR, BAD_BTIMER_TEST_PSC, BAD_BTIMER_TEST_INTR);
    nvic_set_interrupt_priority(TIM6_INTR,NVIC_PRIO14);
    tim_enable(BTIM6);
    nvic_clear_interrupt(TIM6_INTR);
    nvic_enable_interrupt(TIM6_INTR);
    dbgmcu_freeze_apb1l_periphals(DBGMCU, DBGMCU_APB1L_TIM6);
}
void isr_test();

void tim6_usr(){
    isr_test();
}
#endif

void __platform_setup(){
    __main_clock_setup();
    __periph_setup();
    __tick_setup();
#ifdef BAD_RTOS_ISR_TEST
    __timer_setup();
#endif
}
#endif