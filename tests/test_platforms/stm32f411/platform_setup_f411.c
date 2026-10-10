#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

#include "../platform_setup.h" 

#define BAD_USART_IMPLEMENTATION
#define BAD_FLASH_IMPLEMENTATION
#define BAD_RCC_IMPLEMENTATION
#define BAD_GPIO_IMPLEMENTATION
#define BAD_BTIMER_IMPLEMENTATION

#define BTIMER_TIM1_UP_TIM10_ISR_IMPLEMENTATION
#define BTIMER_USE_TIM10_USR

#define BTIMER_TIM1_TRG_COM_TIM11_ISR_IMPLEMENTATION
#define BTIMER_USE_TIM11_USR

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

#define BAD_BTIMER_TEST_ARR    (49999)
#define BAD_BTIMER_TEST_PSC    (20)
#define BAD_BTIMER_TEST_INTR   (BTIMER_UPDATE)

#define BAD_BTIMER_TIMEOUT_ARR    (49999)
#define BAD_BTIMER_TIMEOUT_PSC    (2000)
#define BAD_BTIMER_TIMEOUT_INTR   (BTIMER_UPDATE)

static void (* pperiodic_func)(void);
static void (* ptimeout_func)(void);

static inline void __main_clock_setup()
{
    flash_acceleration_setup(BADHAL_FLASH_LATENCY, FLASH_DCACHE_ENABLE, FLASH_ICACHE_ENABLE);
    rcc_sysclock_setup();
}

static inline void __periph_setup()
{
    rcc_set_ahb1_clocking(BAD_RTOS_AHB1_PERIPEHRALS);
    io_setup_pin(UART_GPIO_PORT, UART1_TX_PIN, MODER_af, UART1_TX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    io_setup_pin(UART_GPIO_PORT, UART1_RX_PIN, MODER_af, UART1_RX_AF, OSPEEDR_high_speed, PUPDR_no_pull, OTYPR_push_pull);
    rcc_set_apb2_clocking(BAD_RTOS_APB2_PERIPHERALS);
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

void __platform_periodic_irq_setup(void (* periodic_func)(void))
{
    basic_timer_setup(BTIM10,0, BAD_BTIMER_TEST_ARR, BAD_BTIMER_TEST_PSC, BAD_BTIMER_TEST_INTR);
    nvic_set_interrupt_priority(NVIC_TIM1_UP_TIM10_INTR,NVIC_PRIO14);
    nvic_clear_interrupt(NVIC_TIM1_UP_TIM10_INTR);
    nvic_disable_interrupt(NVIC_TIM1_UP_TIM10_INTR);
    dbgmcu_freeze_apb2_periphals(DBGMCU, DBGMCU_APB2_TIM10);
    tim_enable(BTIM10);
    pperiodic_func = periodic_func;
}

u32 __platform_get_periodic_irqn()
{
    return NVIC_TIM1_UP_TIM10_INTR; 
}

void tim10_usr(){
    pperiodic_func();
}

void __platform_timeout_setup(void (* timeout_func)(void))
{
    basic_timer_setup(BTIM11, BTIMER_FEATURE_OPM, BAD_BTIMER_TIMEOUT_ARR, BAD_BTIMER_TIMEOUT_PSC, BAD_BTIMER_TIMEOUT_INTR);
    nvic_set_interrupt_priority(NVIC_TIM1_TRG_COM_TIM11_INTR,NVIC_PRIO1);
    nvic_clear_interrupt(NVIC_TIM1_TRG_COM_TIM11_INTR);
    nvic_enable_interrupt(NVIC_TIM1_TRG_COM_TIM11_INTR);
    dbgmcu_freeze_apb2_periphals(DBGMCU, DBGMCU_APB2_TIM11);
    ptimeout_func = timeout_func; 
}

void __platform_start_timeout()
{
    tim_set_cnt(BTIM11,BAD_BTIMER_TIMEOUT_ARR);
    tim_enable(BTIM11);
}

void __platform_pause_timeout()
{
    tim_disable(BTIM11);
}

u8 *__platform_get_timeout_timer_addr()
{
    return (u8 *)BTIM11;
}

u32 __platform_get_timeout_timer_size()
{
    return sizeof(BTIMER_typedef_t);
}

void tim11_usr()
{
    ptimeout_func();
}

u8 *__platform_get_nullptr()
{
    return (u8 *)0;
}

u8* __platform_get_mpu_test_region_addr()
{
    return (u8 *)&(BTIM10->CR1);
}

u32 __platform_get_mpu_test_region_size()
{
    return 32;
}
