#ifndef BAD_RTOS_PLATFORM_SETUP_H
#define BAD_RTOS_PLATFORM_SETUP_H

extern void __platform_base_setup();

extern void __platform_timeout_setup(void (* timeout_func)(void));
extern void __platform_start_timeout();
extern void __platform_pause_timeout();
extern u8 *__platform_get_timeout_timer_addr();
extern u32 __platform_get_timeout_timer_size();

extern void __platform_periodic_irq_setup(void (* periodic_func)(void));
extern u32 __platform_get_periodic_irqn();

extern u8* __platform_get_mpu_test_region_addr();
extern u32 __platform_get_mpu_test_region_size();

#endif
