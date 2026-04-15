/* STARTUP */
#include "badhal_h562.h"
extern unsigned int __estack;

extern unsigned int __rdata;
extern unsigned int __data;
extern unsigned int __edata;

extern unsigned int __bss;
extern unsigned int __ebss;

extern unsigned int __rramfunc;
extern unsigned int __ramfunc;
extern unsigned int __eramfunc;

extern unsigned int __rramfunc;
extern unsigned int __ramfunc;
extern unsigned int __eramfunc;

extern unsigned int __kernel_bss;
extern unsigned int __ekernel_bss;

typedef void (*constructor_ptr)();

extern constructor_ptr __init_array[];
extern constructor_ptr __einit_array[];


extern int main();

void default_isr(){
    while(1);
}
WEAK_ISR(isr_hardfault);
WEAK_ISR(pendsv_isr);
WEAK_ISR(systick_isr);
WEAK_ISR(svc_isr);
WEAK_ISR(wwdg_isr);
WEAK_ISR(pvd_isr);
WEAK_ISR(rtc_isr);
WEAK_ISR(rtc_s_isr);
WEAK_ISR(tamp_isr);
WEAK_ISR(ramcfg_isr);
WEAK_ISR(flash_isr);
WEAK_ISR(flash_s_isr);
WEAK_ISR(gtzc_isr);
WEAK_ISR(rcc_isr);
WEAK_ISR(rcc_s_isr);
WEAK_ISR(exti0_isr);
WEAK_ISR(exti1_isr);
WEAK_ISR(exti2_isr);
WEAK_ISR(exti3_isr);
WEAK_ISR(exti4_isr);
WEAK_ISR(exti5_isr);
WEAK_ISR(exti6_isr);
WEAK_ISR(exti7_isr);
WEAK_ISR(exti8_isr);
WEAK_ISR(exti9_isr);
WEAK_ISR(exti10_isr);
WEAK_ISR(exti11_isr);
WEAK_ISR(exti12_isr);
WEAK_ISR(exti13_isr);
WEAK_ISR(exti14_isr);
WEAK_ISR(exti15_isr);
WEAK_ISR(gpdma1_channel0_isr);
WEAK_ISR(gpdma1_channel1_isr);
WEAK_ISR(gpdma1_channel2_isr);
WEAK_ISR(gpdma1_channel3_isr);
WEAK_ISR(gpdma1_channel4_isr);
WEAK_ISR(gpdma1_channel5_isr);
WEAK_ISR(gpdma1_channel6_isr);
WEAK_ISR(gpdma1_channel7_isr);
WEAK_ISR(iwdg_isr);
WEAK_ISR(adc1_isr);
WEAK_ISR(dac1_isr);
WEAK_ISR(fdcan1_it0_isr);
WEAK_ISR(fdcan1_it1_isr);
WEAK_ISR(tim1_brk_isr);
WEAK_ISR(tim1_up_isr);
WEAK_ISR(tim1_trg_com_isr);
WEAK_ISR(tim1_cc_isr);
WEAK_ISR(tim2_isr);
WEAK_ISR(tim3_isr);
WEAK_ISR(tim4_isr);
WEAK_ISR(tim5_isr);
WEAK_ISR(tim6_isr);
WEAK_ISR(tim7_isr);
WEAK_ISR(i2c1_ev_isr);
WEAK_ISR(i2c1_er_isr);
WEAK_ISR(i2c2_ev_isr);
WEAK_ISR(i2c2_er_isr);
WEAK_ISR(spi1_isr);
WEAK_ISR(spi2_isr);
WEAK_ISR(spi3_isr);
WEAK_ISR(usart1_isr);
WEAK_ISR(usart2_isr);
WEAK_ISR(usart3_isr);
WEAK_ISR(uart4_isr);
WEAK_ISR(uart5_isr);
WEAK_ISR(lpuart1_isr);
WEAK_ISR(lptim1_isr);
WEAK_ISR(tim8_brk_isr);
WEAK_ISR(tim8_up_isr);
WEAK_ISR(tim8_trg_com_isr);
WEAK_ISR(tim8_cc_isr);
WEAK_ISR(adc2_isr);
WEAK_ISR(lptim2_isr);
WEAK_ISR(tim15_isr);
WEAK_ISR(tim16_isr);
WEAK_ISR(tim17_isr);
WEAK_ISR(usb_drd_fs_isr);
WEAK_ISR(crs_isr);
WEAK_ISR(ucpd1_isr);
WEAK_ISR(fmc_isr);
WEAK_ISR(octospi1_isr);
WEAK_ISR(sdmmc1_isr);
WEAK_ISR(i2c3_ev_isr);
WEAK_ISR(i2c3_er_isr);
WEAK_ISR(spi4_isr);
WEAK_ISR(spi5_isr);
WEAK_ISR(spi6_isr);
WEAK_ISR(usart6_isr);
WEAK_ISR(usart10_isr);
WEAK_ISR(usart11_isr);
WEAK_ISR(sai1_isr);
WEAK_ISR(sai2_isr);
WEAK_ISR(gpdma2_channel0_isr);
WEAK_ISR(gpdma2_channel1_isr);
WEAK_ISR(gpdma2_channel2_isr);
WEAK_ISR(gpdma2_channel3_isr);
WEAK_ISR(gpdma2_channel4_isr);
WEAK_ISR(gpdma2_channel5_isr);
WEAK_ISR(gpdma2_channel6_isr);
WEAK_ISR(gpdma2_channel7_isr);
WEAK_ISR(uart7_isr);
WEAK_ISR(uart8_isr);
WEAK_ISR(uart9_isr);
WEAK_ISR(uart12_isr);
WEAK_ISR(fpu_isr);
WEAK_ISR(icache_isr);
WEAK_ISR(dcache1_isr);
WEAK_ISR(dcmi_pssi_isr);
WEAK_ISR(cordic_isr);
WEAK_ISR(fmac_isr);
WEAK_ISR(dts_isr);
WEAK_ISR(rng_isr);
WEAK_ISR(hash_isr);
WEAK_ISR(cec_isr);
WEAK_ISR(tim12_isr);
WEAK_ISR(tim13_isr);
WEAK_ISR(tim14_isr);
WEAK_ISR(i3c1_ev_isr);
WEAK_ISR(i3c1_er_isr);
WEAK_ISR(i2c4_ev_isr);
WEAK_ISR(i2c4_er_isr);
WEAK_ISR(lptim3_isr);
WEAK_ISR(lptim4_isr);
WEAK_ISR(lptim5_isr);
WEAK_ISR(lptim6_isr);

static inline void data_init(){ 
    unsigned int *src = &__rdata;
    unsigned int *dest = &__data;
    while (dest<&__edata) {
        *dest++ = *src++;
    }
}

static inline void bss_init(){
    unsigned int *ksrc = &__kernel_bss;
    while (ksrc < &__ekernel_bss) {
        *ksrc++ = 0; 
    }

    unsigned int *src = &__bss;
    while (src<&__ebss) {
        *src++ = 0; 
    }
}

static inline void ramfunc_init(){
    unsigned int *src = &__rramfunc;
    unsigned int *dest = &__ramfunc;
    while (dest<&__eramfunc) {
        *dest++ = *src++;
    }
}

static inline void constructors_init(){
    constructor_ptr* constructors = __init_array;
    while (constructors < __einit_array) {
        (*constructors)();  // call the constructor
        constructors++;
    }
}

void __attribute__((noreturn)) isr_reset(){
    data_init();
    bss_init();
    ramfunc_init();
    constructors_init();
    main();
    while(1);
};

#define IVT_SIZE (147U)
typedef void (*isr_addr_t) (void);

const isr_addr_t ivt_table[IVT_SIZE] __attribute__((used,section(".ivt")))={ 
    (isr_addr_t)&__estack,
    isr_reset,
    0, //NMI
    isr_hardfault,
    isr_hardfault,
    isr_hardfault,
    isr_hardfault,
    isr_hardfault,
    0,
    0,
    0,
    svc_isr,
    isr_hardfault,
    0,
    pendsv_isr,
    systick_isr,
    wwdg_isr,
    pvd_isr,
    rtc_isr,
    rtc_s_isr,
    tamp_isr,
    ramcfg_isr,
    flash_isr,
    flash_s_isr,
    gtzc_isr,
    rcc_isr,
    rcc_s_isr,
    exti0_isr,
    exti1_isr,
    exti2_isr,
    exti3_isr,
    exti4_isr,
    exti5_isr,
    exti6_isr,
    exti7_isr,
    exti8_isr,
    exti9_isr,
    exti10_isr,
    exti11_isr,
    exti12_isr,
    exti13_isr,
    exti14_isr,
    exti15_isr,
    gpdma1_channel0_isr,
    gpdma1_channel1_isr,
    gpdma1_channel2_isr,
    gpdma1_channel3_isr,
    gpdma1_channel4_isr,
    gpdma1_channel5_isr,
    gpdma1_channel6_isr,
    gpdma1_channel7_isr,
    iwdg_isr,
    0,
    adc1_isr,
    dac1_isr,
    fdcan1_it0_isr,
    fdcan1_it1_isr,
    tim1_brk_isr,
    tim1_up_isr,
    tim1_trg_com_isr,
    tim1_cc_isr,
    tim2_isr,
    tim3_isr,
    tim4_isr,
    tim5_isr,
    tim6_isr,
    tim7_isr,
    i2c1_ev_isr,
    i2c1_er_isr,
    i2c2_ev_isr,
    i2c2_er_isr,
    spi1_isr,
    spi2_isr,
    spi3_isr,
    usart1_isr,
    usart2_isr,
    usart3_isr,
    uart4_isr,
    uart5_isr,
    lpuart1_isr,
    lptim1_isr,
    tim8_brk_isr,
    tim8_up_isr,
    tim8_trg_com_isr,
    tim8_cc_isr,
    adc2_isr,
    lptim2_isr,
    tim15_isr,
    tim16_isr,
    tim17_isr,
    usb_drd_fs_isr,
    crs_isr,
    ucpd1_isr,
    fmc_isr,
    octospi1_isr,
    sdmmc1_isr,
    i2c3_ev_isr,
    i2c3_er_isr,
    spi4_isr,
    spi5_isr,
    spi6_isr,
    usart6_isr,
    usart10_isr,
    usart11_isr,
    sai1_isr,
    sai2_isr,
    gpdma2_channel0_isr,
    gpdma2_channel1_isr,
    gpdma2_channel2_isr,
    gpdma2_channel3_isr,
    gpdma2_channel4_isr,
    gpdma2_channel5_isr,
    gpdma2_channel6_isr,
    gpdma2_channel7_isr,
    uart7_isr,
    uart8_isr,
    uart9_isr,
    uart12_isr,
    0,
    fpu_isr,
    icache_isr,
    dcache1_isr,
    0,
    0,
    dcmi_pssi_isr,
    0,
    0,
    cordic_isr,
    fmac_isr,
    dts_isr,
    rng_isr,
    0,
    0,
    hash_isr,
    0,
    cec_isr,
    tim12_isr,
    tim13_isr,
    tim14_isr,
    i3c1_ev_isr,
    i3c1_er_isr,
    i2c4_ev_isr,
    i2c4_er_isr,
    lptim3_isr,
    lptim4_isr,
    lptim5_isr,
    lptim6_isr
};

