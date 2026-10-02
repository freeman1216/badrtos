#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#include "../platform_setup.h" 

#define BAD_USART_IMPLEMENTATION
#define BAD_RCC_IMPLEMENTATION
#define BAD_FLASH_IMPLEMENTATION
#define BAD_GPIO_IMPLEMENTATION
#define BAD_PWR_IMPLEMENTATION
#define BAD_BTIMER_IMPLEMENTATION

#define BTIMER_TIM6_ISR_IMPLEMENTATION
#define BTIMER_TIM7_ISR_IMPLEMENTATION

#define BAD_HARDFAULT_ISR_IMPLEMENTATION
#define BAD_HARDFAULT_USE_UART

#include "badhal_h562.h"

#define UART_GPIO_PORT          (GPIOA)
#define UART1_TX_PIN            (9)
#define UART1_RX_PIN            (10)
#define UART1_TX_AF             (7)
#define UART1_RX_AF             (7)

#define BAD_BTIMER_TEST_ARR    (75)
#define BAD_BTIMER_TEST_PSC    (31999)
#define BAD_BTIMER_TEST_INTR   (BTIMER_UPDATE)

#define BAD_BTIMER_TIMEOUT_ARR    (3750)
#define BAD_BTIMER_TIMEOUT_PSC    (63999)
#define BAD_BTIMER_TIMEOUT_INTR   (BTIMER_UPDATE)

#define BAD_RTOS_FLASH_LATENCY    (FLASH_LATENCY_5ws)

#define BAD_RTOS_APB1L_PERIPHERALS (RCC_APB1L_TIM6 | RCC_APB1L_TIM7)

#define BAD_RTOS_AHB2_PERIPEHRALS    (RCC_AHB2_GPIOA|RCC_AHB2_GPIOC|RCC_AHB2_SRAM3|RCC_AHB2_SRAM2)
#define BAD_RTOS_APB2_PERIPHERALS    (RCC_APB2_USART1)

#define BAD_RTOS_SETTINGS (USART_FEATURE_RECIEVE_EN|USART_FEATURE_TRANSMIT_EN)

static void (* pperiodic_func)(void);
static void (* ptimeout_func)(void);

static inline void __main_clock_setup()
{
    flash_acceleration_setup(FLASH_REGS,BAD_RTOS_FLASH_LATENCY, FLASH_PROGRAMMING_DELAY_2);
    pwr_setup_vos(PWR,PWR_VOS0);
    rcc_default_sysclock_setup(RCC);
}

static inline void __periph_setup()
{
    rcc_set_ahb2_clocking(RCC,BAD_RTOS_AHB2_PERIPEHRALS);
    io_setup_pin(UART_GPIO_PORT, UART1_TX_PIN, MODER_af, UART1_TX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    io_setup_pin(UART_GPIO_PORT, UART1_RX_PIN, MODER_af, UART1_RX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    //Enable UART clocking
    rcc_set_apb2_clocking(RCC,BAD_RTOS_APB2_PERIPHERALS);
    rcc_set_apb1l_clocking(RCC,BAD_RTOS_APB1L_PERIPHERALS);
}

static inline void __tick_setup()
{
    scb_set_core_interrupt_priority(SCB_SYSTICK_INTR,SCB_PRIO15);
    systick_setup(CLOCK_SPEED/1000, SYSTICK_FEATURE_CLOCK_SOURCE|SYSTICK_FEATURE_TICK_INTERRUPT);
    systick_enable();
}

void __platform_base_setup()
{
    __main_clock_setup();
    __periph_setup();
    __tick_setup();
}

void __platform_periodic_irq_setup( void (* periodic_func)(void))
{
    basic_timer_setup(BTIM6, 0,BAD_BTIMER_TEST_ARR, BAD_BTIMER_TEST_PSC, BAD_BTIMER_TEST_INTR);
    nvic_set_interrupt_priority(TIM6_INTR,NVIC_PRIO14);
    nvic_clear_interrupt(TIM6_INTR);
    nvic_disable_interrupt(TIM6_INTR);
    dbgmcu_freeze_apb1l_periphals(DBGMCU, DBGMCU_APB1L_TIM6);
    tim_enable(BTIM6);
    pperiodic_func = periodic_func;
}

u32 __platform_get_periodic_irqn()
{
    return TIM6_INTR - 16; 
}

void tim6_usr()
{
    pperiodic_func();
}

void __platform_timeout_setup(void (* timeout_func)(void))
{
    basic_timer_setup(BTIM7, BTIMER_FEATURE_OPM, BAD_BTIMER_TIMEOUT_ARR, BAD_BTIMER_TIMEOUT_PSC, BAD_BTIMER_TIMEOUT_INTR);
    nvic_set_interrupt_priority(TIM7_INTR,NVIC_PRIO1);
    nvic_clear_interrupt(TIM7_INTR);
    nvic_enable_interrupt(TIM7_INTR);
    dbgmcu_freeze_apb1l_periphals(DBGMCU, DBGMCU_APB1L_TIM7);
    ptimeout_func = timeout_func;
}

void __platform_start_timeout()
{
    tim_set_cnt(BTIM7,0);
    tim_enable(BTIM7);
}

void __platform_pause_timeout()
{
    tim_disable(BTIM7);
}

u8 *__platform_get_timeout_timer_addr()
{
    return (u8 *)BTIM7;
}

u32 __platform_get_timeout_timer_size()
{
    return sizeof(BTIMER_typedef_t);
}

void tim7_usr()
{
    ptimeout_func();
}

u8 *__platform_get_nullptr()
{
    return (u8 *)0;
}

u8 *__platform_get_mpu_test_region_addr()
{
    return (u8 *)&(BTIM6->CR1);
}

u32 __platform_get_mpu_test_region_size()
{
    return 32;
}
