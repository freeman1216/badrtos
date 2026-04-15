#pragma once
#ifndef BAD_HAL_H
#define BAD_HAL_H

#include <stdint.h>

//GLOBAL CONFIG
//Core
#define BAD_HAL_USE_SCB
#define BAD_HAL_USE_MPU
//#define BAD_HAL_USE_NVIC
#define BAD_HAL_USE_SYSTICK
//#define BAD_HAL_USE_FPU
//Peripherals
#define BAD_HAL_USE_FLASH
#define BAD_HAL_USE_RCC
#define BAD_HAL_USE_USART
#define BAD_HAL_USE_GPIO
#define BAD_HAL_USE_PWR
//common defines

#define __IO volatile

#define BAD_RCC_DEFAULT_SETTINGS (PLL_FEATURE_SOURCE_HSE|PLL_FEATURE_REF_8_16_MHZ|PLL_FEATURE_PDIV_EN|PLL_FEATURE_RDIV_EN|PLL_FEATURE_QDIV_EN)
#define BAD_PLLN        (240)
#define BAD_PLLM        (8)
#define BAD_PLLR        (2)
#define BAD_PLLQ        (10)
#define BAD_PLLP        (2)
#define BAD_HSE_SPEED   (16000000UL)

#define BAD_HPRE    (HPRE_DIV_1)
#define BAD_PPRE1   (PPRE_DIV_1)
#define BAD_PPRE2   (PPRE_DIV_1)
#define BAD_PPRE3   (PPRE_DIV_1)

#define CLOCK_SPEED (((BAD_HSE_SPEED / BAD_PLLM) * BAD_PLLN) / BAD_PLLP)      //240MHZ
//hw interrupts (triggered by hardware and handled in drivers)
#define STRONG_ISR(x) void x(void)
#define WEAK_ISR(x) void x(void) __attribute__((weak, alias("default_isr")))
#define STRONG_USER_ISR(x,...) void x(__VA_ARGS__)
#define WEAK_USER_ISR(x,...) void x(__VA_ARGS__) __attribute__((weak, alias(#x"_default")))
#define DEFAULT_USER_ISR(x,...) void x##_default(__VA_ARGS__)
#define WEAK_PERIPH_USER_ISR(x,default_isr,...) void x(__VA_ARGS__) __attribute__((weak, alias(#default_isr"_default")))
#define ATTR_RAMFUNC __attribute__((section(".ramfunc")))
#define ALWAYS_STATIC static inline
#define UNUSED(x) (void)x

#define OPT_BARRIER asm volatile("": : :"memory")
#define DSB __asm volatile("dsb":::"memory")
#define DMB __asm volatile("dmb":::"memory")
#define ISB __asm volatile("isb":::"memory")
#define __ENABLE_INTERUPTS __asm volatile ("cpsie i":::"memory")
#define __DISABLE_INTERUPTS __asm volatile ("cpsid i":::"memory")

//Core


//SCB
#ifdef BAD_HAL_USE_SCB

typedef struct{
    uint32_t CPUID;                  /*!< Offset: 0x000 (R/ )  CPUID Base Register */
    __IO uint32_t ICSR;                   /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register */
    __IO uint32_t VTOR;                   /*!< Offset: 0x008 (R/W)  Vector Table Offset Register */
    __IO uint32_t AIRCR;                  /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
    __IO uint32_t SCR;                    /*!< Offset: 0x010 (R/W)  System Control Register */
    __IO uint32_t CCR;                    /*!< Offset: 0x014 (R/W)  Configuration Control Register */
    __IO uint8_t  SHP[12U];              /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
    __IO uint32_t SHCSR;                  /*!< Offset: 0x024 (R/W)  System Handler Control and State Register */
    __IO uint32_t CFSR;                   /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register */
    __IO uint32_t HFSR;                   /*!< Offset: 0x02C (R/W)  HardFault Status Register */
    __IO uint32_t DFSR;                   /*!< Offset: 0x030 (R/W)  Debug Fault Status Register */
    __IO uint32_t MMFAR;                  /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register */
    __IO uint32_t BFAR;                   /*!< Offset: 0x038 (R/W)  BusFault Address Register */
    __IO uint32_t AFSR;                   /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register */
    uint32_t ID_PFR[2U];             /*!< Offset: 0x040 (R/ )  Processor Feature Register */
    uint32_t ID_DFR;                 /*!< Offset: 0x048 (R/ )  Debug Feature Register */
    uint32_t ID_AFR;                 /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register */
    uint32_t ID_MMFR[4U];            /*!< Offset: 0x050 (R/ )  Memory Model Feature Register */
    uint32_t ID_ISAR[6U];            /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register */
    uint32_t CLIDR;                  /*!< Offset: 0x078 (R/ )  Cache Level ID register */
    uint32_t CTR;                    /*!< Offset: 0x07C (R/ )  Cache Type register */
    uint32_t CCSIDR;                 /*!< Offset: 0x080 (R/ )  Cache Size ID Register */
    __IO uint32_t CSSELR;                 /*!< Offset: 0x084 (R/W)  Cache Size Selection Register */
    __IO uint32_t CPACR;                  /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
    __IO uint32_t NSACR;                  /*!< Offset: 0x08C (R/W)  Non-Secure Access Control Register */
    uint32_t RESERVED7[21U];
    __IO uint32_t SFSR;                   /*!< Offset: 0x0E4 (R/W)  Secure Fault Status Register */
    __IO uint32_t SFAR;                   /*!< Offset: 0x0E8 (R/W)  Secure Fault Address Register */
    uint32_t RESERVED3[69U];
    __IO  uint32_t STIR;                   /*!< Offset: 0x200 ( /W)  Software Triggered Interrupt Register */
    uint32_t RESERVED4[15U];
    uint32_t MVFR0;                  /*!< Offset: 0x240 (R/ )  Media and VFP Feature Register 0 */
    uint32_t MVFR1;                  /*!< Offset: 0x244 (R/ )  Media and VFP Feature Register 1 */
    uint32_t MVFR2;                  /*!< Offset: 0x248 (R/ )  Media and VFP Feature Register 2 */
    uint32_t RESERVED5[1U];
    __IO uint32_t ICIALLU;/*!< Offset: 0x250 ( /W)I-Cache Invalidate All to PoU */
    uint32_t RESERVED6[1U];
    __IO uint32_t ICIMVAU;/*!< Offset: 0x258 ( /W)I-Cache Invalidate by MVA to PoU */
    __IO uint32_t DCIMVAC;/*!< Offset: 0x25C ( /W)D-Cache Invalidate by MVA to PoC */
    __IO uint32_t DCISW;/*!< Offset: 0x260 ( /W)D-Cache Invalidate by Set-way */
    __IO uint32_t DCCMVAU;/*!< Offset: 0x264 ( /W)D-Cache Clean by MVA to PoU */
    __IO uint32_t DCCMVAC;/*!< Offset: 0x268 ( /W)D-Cache Clean by MVA to PoC */
    __IO uint32_t DCCSW;/*!< Offset: 0x26C ( /W)D-Cache Clean by Set-way */
    __IO uint32_t DCCIMVAC; /*!< Offset: 0x270 ( /W)D-Cache Clean and Invalidate by MVA to PoC */
    __IO uint32_t DCCISW; /*!< Offset: 0x274 ( /W)D-Cache Clean and Invalidate by Set-way */
    __IO uint32_t BPIALL; /*!< Offset: 0x278 ( /W)Branch Predictor Invalidate All */
} SCB_typedef_t;


typedef enum{
    SCB_PRIO_GROUP0 = 0,
    SCB_PRIO_GROUP1,
    SCB_PRIO_GROUP2,
    SCB_PRIO_GROUP3,
    SCB_PRIO_GROUP4
}SCB_prio_grouping_t;

typedef enum{
    SCB_MEMORY_MANAGEMENT_INTR = 0,
    SCB_BUS_FAULT_INTR=1,
    SCB_USAGE_FAULT_INTR=2,
    SCB_SVC_INTR = 7,
    SCB_DEBUG_MONITOR_INTR = 8,
    SCB_PENDSV_INTR = 10,
    SCB_SYSTICK_INTR = 11
}SCB_core_interrupt_t;

typedef enum {
    SCB_PRIO0 = 0,
    SCB_PRIO1,
    SCB_PRIO2,
    SCB_PRIO3,
    SCB_PRIO4,
    SCB_PRIO5,
    SCB_PRIO6,
    SCB_PRIO7,
    SCB_PRIO8,
    SCB_PRIO9,
    SCB_PRIO10,
    SCB_PRIO11,
    SCB_PRIO12,
    SCB_PRIO13,
    SCB_PRIO14,
    SCB_PRIO15
}SCB_interrupt_priority_t;

typedef enum{
    SCB_FPU_NO_ACCESS = 0,
    SCB_FPU_PRIV_ACCESS = 5,
    SCB_FPU_FULL_ACCESS = 15,
}SCB_FPU_permission_t;

#define SCB ((SCB_typedef_t *) 0xE000ED00UL)

#define SCB_AIRCR_VECTKEY_SHIFT             16U                                            
#define SCB_AIRCR_VECTKEY_MASK              (0xFFFF << 16)            
#define SCB_ICSR_PENDSVSET                  (0x1 << 28 ) 
#define SCB_AIRCR_PRIGROUP_SHIFT            8                                            
#define SCB_AIRCR_PRIGROUP_MASK             (7 << SCB_AIRCR_PRIGROUP_SHIFT)                
#define SCB_CPACR_FPU_SHIFT                 20U
#define SCB_CPACR_FPU_MASK                  (0xF << SCB_CPACR_FPU_SHIFT)

ALWAYS_STATIC void scb_trigger_pendsv(){
    SCB->ICSR = SCB_ICSR_PENDSVSET;
}

ALWAYS_STATIC void scb_set_priority_grouping(SCB_prio_grouping_t prio){
    uint32_t reg_value  =  SCB->AIRCR;                                                
    reg_value &= ~(SCB_AIRCR_VECTKEY_MASK | SCB_AIRCR_PRIGROUP_MASK);  
    reg_value  =  (reg_value | (0x5FA << SCB_AIRCR_VECTKEY_SHIFT) | (prio << SCB_AIRCR_PRIGROUP_SHIFT));              
    SCB->AIRCR =  reg_value;
    DSB;
    OPT_BARRIER;
}

ALWAYS_STATIC void scb_set_core_interrupt_priority(SCB_core_interrupt_t intr,SCB_interrupt_priority_t prio){
    SCB->SHP[intr] = prio << 4;
}

ALWAYS_STATIC void scb_set_fpu_permission_level(SCB_FPU_permission_t perms){
    SCB->CPACR &= ~(SCB_CPACR_FPU_MASK);
    SCB->CPACR |= perms << SCB_CPACR_FPU_SHIFT;
    DSB;
    ISB;
} 

#endif // BAD_HAL_USE_SCB

#ifdef BAD_HAL_USE_MPU

typedef struct
{
  __IO uint32_t TYPE;                   /*!< Offset: 0x000 (R/ )  MPU Type Register */
  __IO uint32_t CTRL;                   /*!< Offset: 0x004 (R/W)  MPU Control Register */
  __IO uint32_t RNR;                    /*!< Offset: 0x008 (R/W)  MPU Region Number Register */
  __IO uint32_t RBAR;                   /*!< Offset: 0x00C (R/W)  MPU Region Base Address Register */
  __IO uint32_t RLAR;                   /*!< Offset: 0x010 (R/W)  MPU Region Limit Address Register */
  __IO uint32_t RBAR_A1;                /*!< Offset: 0x014 (R/W)  MPU Region Base Address Register Alias 1 */
  __IO uint32_t RLAR_A1;                /*!< Offset: 0x018 (R/W)  MPU Region Limit Address Register Alias 1 */
  __IO uint32_t RBAR_A2;                /*!< Offset: 0x01C (R/W)  MPU Region Base Address Register Alias 2 */
  __IO uint32_t RLAR_A2;                /*!< Offset: 0x020 (R/W)  MPU Region Limit Address Register Alias 2 */
  __IO uint32_t RBAR_A3;                /*!< Offset: 0x024 (R/W)  MPU Region Base Address Register Alias 3 */
  __IO uint32_t RLAR_A3;                /*!< Offset: 0x028 (R/W)  MPU Region Limit Address Register Alias 3 */
    uint32_t RESERVED0[1];
  __IO uint32_t MAIR[2];

} MPU_typedef_t;


#define MPU_CTRL_ENABLE (0x1)
#define MPU_CTRL_DEFAULT_MAP (0x4)
#define MPU_BASE (0xE000ED90UL)

#define MPU ((MPU_typedef_t *)MPU_BASE)

ALWAYS_STATIC void mpu_enable_with_default_map(){
    DMB;
    MPU->CTRL = MPU_CTRL_ENABLE | MPU_CTRL_DEFAULT_MAP;
    DSB;
    ISB;
}
#endif


#ifdef BAD_HAL_USE_FPU
typedef struct{
    __IO uint32_t FPCCR;
    __IO uint32_t FPCAR;
    __IO uint32_t FPDCR;
    __IO uint32_t MVFR0;
    __IO uint32_t MVFR1;
}FPU_typedef_t;

#define FPU_BASE (0xE000EF34)
#define FPU ((__IO FPU_typedef_t *) FPU_BASE)

typedef enum {
    FPU_FEATURE_DISALOW_UNPRIV_CHANGE = 0,
    FPU_FEATURE_ALLOW_UNPRIV_CHANGE = 1,
    FPU_FEATURE_DISABLE_AUTO_STACKING = 0,
    FPU_FEATURE_ENABLE_AUTO_STACKING = 0x80000000,
    FPU_FEATURE_DISABLE_LAZY_STACKING = 0,
    FPU_FEATURE_ENABLE_LAZY_STACKING = 0x40000000
}FPU_features_t;

ALWAYS_STATIC void fpu_setup( FPU_features_t features){
    FPU->FPCCR = features;
    DSB;
    ISB;
}

#endif // BAD_HAL_USE_FPU

//Flash
#ifdef BAD_HAL_USE_FLASH

#ifdef BAD_FLASH_IMPLEMENTATION

typedef struct {                                /*!< FLASH Structure                                                           */
    __IO uint32_t  ACR;                          /*!< FLASH access control register                                             */
    __IO uint32_t  NSKEYR;                       /*!< FLASH non-secure key register                                             */
    __IO uint32_t  SECKEYR;                      /*!< FLASH secure key register                                                 */
    __IO uint32_t  OPTKEYR;                      /*!< FLASH option key register                                                 */
    __IO uint32_t  NSOBKKEYR;                    /*!< FLASH non-secure OBK key register                                         */
    __IO uint32_t  SECOBKKEYR;                   /*!< FLASH secure OBK key register                                             */
    __IO uint32_t  OPSR;                         /*!< FLASH operation status register                                           */
    __IO uint32_t  OPTCR;                        /*!< FLASH option control register                                             */
    __IO uint32_t  NSSR;                         /*!< FLASH non-secure status register                                          */
    __IO uint32_t  SECSR;                        /*!< FLASH secure status register                                              */
    __IO uint32_t  NSCR;                         /*!< FLASH non-secure control register                                         */
    __IO uint32_t  SECCR;                        /*!< FLASH secure control register                                             */
    __IO uint32_t  NSCCR;                        /*!< FLASH non-secure clear control register                                   */
    __IO uint32_t  SECCCR;                       /*!< FLASH secure clear control register                                       */
    uint32_t  RESERVED;
    __IO uint32_t  PRIVCFGR;                     /*!< FLASH privilege configuration register                                    */
    __IO uint32_t  NSOBKCFGR;                    /*!< FLASH non-secure OBK configuration register                               */
    __IO uint32_t  SECOBKCFGR;                   /*!< FLASH secure OBK configuration register                                   */
    __IO uint32_t  HDPEXTR;                      /*!< FLASH HDP extension register                                              */
    uint32_t  RESERVED1;
    __IO uint32_t  OPTSR_CUR;                    /*!< FLASH option status register                                              */
    __IO uint32_t  OPTSR_PRG;                    /*!< FLASH option status register                                              */
    uint32_t  RESERVED2[2];
    __IO uint32_t  NSEPOCHR_CUR;                 /*!< FLASH non-secure EPOCH register                                           */
    uint32_t  RESERVED3;
    __IO uint32_t  SECEPOCHR_CUR;                /*!< FLASH secure EPOCH register                                               */
    uint32_t  RESERVED4;
    __IO uint32_t  OPTSR2_CUR;                   /*!< FLASH option status register 2                                            */
    __IO uint32_t  OPTSR2_PRG;                   /*!< FLASH option status register 2                                            */
    uint32_t  RESERVED5[2];
    __IO uint32_t  NSBOOTR_CUR;                  /*!< FLASH non-secure boot register                                            */
    __IO uint32_t  NSBOOTR_PRG;                  /*!< FLASH non-secure boot register                                            */
    __IO uint32_t  SECBOOTR_CUR;                 /*!< FLASH secure boot register                                                */
    __IO uint32_t  BOOTR_PRG;                    /*!< FLASH secure boot register                                                */
    __IO uint32_t  OTPBLR_CUR;                   /*!< FLASH non-secure OTP block lock                                           */
    __IO uint32_t  OTPBLR_PRG;                   /*!< FLASH non-secure OTP block lock                                           */
    uint32_t  RESERVED6[2];
    __IO uint32_t  SECBB1R1;                     /*!< FLASH secure block based register for Bank 1                              */
    __IO uint32_t  SECBB1R2;                     /*!< FLASH secure block based register for Bank 1                              */
    __IO uint32_t  SECBB1R3;                     /*!< FLASH secure block based register for Bank 1                              */
    __IO uint32_t  SECBB1R4;                     /*!< FLASH secure block based register for Bank 1                              */
    uint32_t  RESERVED7[4];
    __IO uint32_t  PRIVBB1R1;                    /*!< FLASH privilege block based register for Bank 1                           */
    __IO uint32_t  PRIVBB1R2;                    /*!< FLASH privilege block based register for Bank 1                           */
    __IO uint32_t  PRIVBB1R3;                    /*!< FLASH privilege block based register for Bank 1                           */
    __IO uint32_t  PRIVBB1R4;                    /*!< FLASH privilege block based register for Bank 1                           */
    uint32_t  RESERVED8[4];
    __IO uint32_t  SECWM1R_CUR;                  /*!< FLASH security watermark for Bank 1                                       */
    __IO uint32_t  SECWM1R_PRG;                  /*!< FLASH security watermark for Bank 1                                       */
    __IO uint32_t  WRP1R_CUR;                    /*!< FLASH write sector group protection for Bank 1                            */
    __IO uint32_t  WRP1R_PRG;                    /*!< FLASH write sector group protection for Bank 1                            */
    __IO uint32_t  EDATA1R_CUR;                  /*!< FLASH data sector configuration Bank 1                                    */
    __IO uint32_t  EDATA1R_PRG;                  /*!< FLASH data sector configuration Bank 1                                    */
    __IO uint32_t  HDP1R_CUR;                    /*!< FLASH HDP Bank 1 configuration                                            */
    __IO uint32_t  HDP1R_PRG;                    /*!< FLASH HDP Bank 1 configuration                                            */
    __IO uint32_t  ECCCORR;                      /*!< FLASH ECC correction register                                             */
    __IO uint32_t  ECCDETR;                      /*!< FLASH ECC detection register                                              */
    __IO uint32_t  ECCDR;                        /*!< FLASH ECC data                                                            */
    uint32_t  RESERVED9[37];
    __IO uint32_t  SECBB2R1;                     /*!< FLASH secure block-based register for Bank 2                              */
    __IO uint32_t  SECBB2R2;                     /*!< FLASH secure block-based register for Bank 2                              */
    __IO uint32_t  SECBB2R3;                     /*!< FLASH secure block-based register for Bank 2                              */
    __IO uint32_t  SECBB2R4;                     /*!< FLASH secure block-based register for Bank 2                              */
    uint32_t  RESERVED10[4];
    __IO uint32_t  PRIVBB2R1;                    /*!< FLASH privilege block-based register for Bank 2                           */
    __IO uint32_t  PRIVBB2R2;                    /*!< FLASH privilege block-based register for Bank 2                           */
    __IO uint32_t  PRIVBB2R3;                    /*!< FLASH privilege block-based register for Bank 2                           */
    __IO uint32_t  PRIVBB2R4;                    /*!< FLASH privilege block-based register for Bank 2                           */
    uint32_t  RESERVED11[4];
    __IO uint32_t  SECWM2R_CUR;                  /*!< FLASH security watermark for Bank 2                                       */
    __IO uint32_t  SECWM2R_PRG;                  /*!< FLASH security watermark for Bank 2                                       */
    __IO uint32_t  WRP2R_CUR;                    /*!< FLASH write sector group protection for Bank 2                            */
    __IO uint32_t  WRP2R_PRG;                    /*!< FLASH write sector group protection for Bank 2                            */
    __IO uint32_t  EDATA2R_CUR;                  /*!< FLASH data sectors configuration Bank 2                                   */
    __IO uint32_t  EDATA2R_PRG;                  /*!< FLASH data sector configuration Bank 2                                    */
    __IO uint32_t  HDP2R_CUR;                    /*!< FLASH HDP Bank 2 configuration                                            */
    __IO uint32_t  HDP2R_PRG;                    /*!< FLASH HDP Bank 2 configuration                                            */
}FLASH_regs_typedef_t;

typedef enum{
    FLASH_LATENCY_0ws   = 0x0,
    FLASH_LATENCY_1ws   = 0x1,
    FLASH_LATENCY_2ws   = 0x2,
    FLASH_LATENCY_3ws   = 0x3,
    FLASH_LATENCY_4ws   = 0x4,
    FLASH_LATENCY_5ws   = 0x5,
    FLASH_LATENCY_6ws   = 0x6,
    FLASH_LATENCY_7ws   = 0x7,
    FLASH_LATENCY_8ws   = 0x8,
    FLASH_LATENCY_9ws   = 0x9,
    FLASH_LATENCY_10ws  = 0xA,
    FLASH_LATENCY_11ws  = 0xB,
    FLASH_LATENCY_12ws  = 0xC,
    FLASH_LATENCY_13ws  = 0xD,
    FLASH_LATENCY_14ws  = 0xE,
    FLASH_LATENCY_15ws  = 0xF
}FLASH_latency_t;

typedef enum{
    FLASH_PROGRAMMING_DELAY_0 = 0x0,
    FLASH_PROGRAMMING_DELAY_1 = 0x10,
    FLASH_PROGRAMMING_DELAY_2 = 0x20
}FLASH_programming_delay_t;

typedef enum{
    FLASH_PREFETCH_DISABLE    = 0x0, 
    FLASH_PREFETCH_ENABLE     = 0x100
}FLASH_prefetch_state_t;

#define FLASH_REGS_BASE         (0x40022000UL)
#define SEC_FLASH_REGS_BASE     (0x50022000UL)

#define FLASH_REGS ((__IO FLASH_regs_typedef_t *)FLASH_REGS_BASE)
#define SECFLASH_REGS ((__IO FLASH_regs_typedef_t *)SEC_FLASH_REG_BASE)

extern void flash_acceleration_setup(__IO FLASH_regs_typedef_t *regs, FLASH_latency_t latency, FLASH_programming_delay_t delay){
    regs->ACR = latency | delay;
}

#endif

#endif // BAD_HAL_USE_FLASH

//RCC
#ifdef BAD_HAL_USE_RCC

#ifdef BAD_RCC_IMPLEMENTATION

typedef struct {                                /*!< RCC Structure                                                             */
    __IO uint32_t  CR;                           /*!< RCC clock control register                                                */
    uint32_t  RESERVED[3];
    __IO uint32_t  HSICFGR;                      /*!< RCC HSI calibration register                                              */
    __IO uint32_t  CRRCR;                        /*!< RCC clock recovery RC register                                            */
    __IO uint32_t  CSICFGR;                      /*!< RCC CSI calibration register                                              */
    __IO uint32_t  CFGR1;                        /*!< RCC clock configuration register                                          */
    __IO uint32_t  CFGR2;                        /*!< RCC CPU domain clock configuration register 2                             */
    uint32_t  RESERVED1;
    __IO uint32_t  PLL1CFGR;                     /*!< RCC PLL clock source selection register                                   */
    __IO uint32_t  PLL2CFGR;                     /*!< RCC PLL clock source selection register                                   */
    __IO uint32_t  PLL3CFGR;                     /*!< RCC PLL clock source selection register                                   */
    __IO uint32_t  PLL1DIVR;                     /*!< RCC PLL1 dividers register                                                */
    __IO uint32_t  PLL1FRACR;                    /*!< RCC PLL1 fractional divider register                                      */
    __IO uint32_t  PLL2DIVR;                     /*!< RCC PLL1 dividers register                                                */
    __IO uint32_t  PLL2FRACR;                    /*!< RCC PLL2 fractional divider register                                      */
    __IO uint32_t  PLL3DIVR;                     /*!< RCC PLL3 dividers register                                                */
    __IO uint32_t  PLL3FRACR;                    /*!< RCC PLL3 fractional divider register                                      */
    uint32_t  RESERVED2;
    __IO uint32_t  CIER;                         /*!< RCC clock source interrupt enable register                                */
    __IO uint32_t  CIFR;                         /*!< RCC clock source interrupt flag register                                  */
    __IO uint32_t  CICR;                         /*!< RCC clock source interrupt clear register                                 */
    uint32_t  RESERVED3;
    __IO uint32_t  AHB1RSTR;                     /*!< RCC AHB1 reset register                                                   */
    __IO uint32_t  AHB2RSTR;                     /*!< RCC AHB2 peripheral reset register                                        */
    uint32_t  RESERVED4;
    __IO uint32_t  AHB4RSTR;                     /*!< RCC AHB4 peripheral reset register                                        */
    uint32_t  RESERVED5;
    __IO uint32_t  APB1LRSTR;                    /*!< RCC APB1 peripheral low reset register                                    */
    __IO uint32_t  APB1HRSTR;                    /*!< RCC APB1 peripheral high reset register                                   */
    __IO uint32_t  APB2RSTR;                     /*!< RCC APB2 peripheral reset register                                        */
    __IO uint32_t  APB3RSTR;                     /*!< RCC APB4 peripheral reset register                                        */
    uint32_t  RESERVED6;
    __IO uint32_t  AHB1ENR;                      /*!< RCC AHB1 peripherals clock register                                       */
    __IO uint32_t  AHB2ENR;                      /*!< RCC AHB2 peripheral clock register                                        */
    uint32_t  RESERVED7;
    __IO uint32_t  AHB4ENR;                      /*!< RCC AHB4 peripheral clock register                                        */
    uint32_t  RESERVED8;
    __IO uint32_t  APB1LENR;                     /*!< RCC APB1 peripheral clock register                                        */
    __IO uint32_t  APB1HENR;                     /*!< RCC APB1 peripheral clock register                                        */
    __IO uint32_t  APB2ENR;                      /*!< RCC APB2 peripheral clock register                                        */
    __IO uint32_t  APB3ENR;                      /*!< RCC APB4 peripheral clock register                                        */
    uint32_t  RESERVED9;
    __IO uint32_t  AHB1LPENR;                    /*!< RCC AHB1 sleep clock register                                             */
    __IO uint32_t  AHB2LPENR;                    /*!< RCC AHB2 sleep clock register                                             */
    uint32_t  RESERVED10;
    __IO uint32_t  AHB4LPENR;                    /*!< RCC AHB4 sleep clock register                                             */
    uint32_t  RESERVED11;
    __IO uint32_t  APB1LLPENR;                   /*!< RCC APB1 sleep clock register                                             */
    __IO uint32_t  APB1HLPENR;                   /*!< RCC APB1 sleep clock register                                             */
    __IO uint32_t  APB2LPENR;                    /*!< RCC APB2 sleep clock register                                             */
    __IO uint32_t  APB3LPENR;                    /*!< RCC APB4 sleep clock register                                             */
    uint32_t  RESERVED12;
    __IO uint32_t  CCIPR1;                       /*!< RCC kernel clock configuration register                                   */
    __IO uint32_t  CCIPR2;                       /*!< RCC kernel clock configuration register                                   */
    __IO uint32_t  CCIPR3;                       /*!< RCC kernel clock configuration register                                   */
    __IO uint32_t  CCIPR4;                       /*!< RCC kernel clock configuration register                                   */
    __IO uint32_t  CCIPR5;                       /*!< RCC kernel clock configuration register                                   */
    uint32_t  RESERVED13;
    __IO uint32_t  BDCR;                         /*!< RCC Backup domain control register                                        */
    __IO uint32_t  RSR;                          /*!< RCC reset status register                                                 */
    uint32_t  RESERVED14[6];
    __IO uint32_t  SECCFGR;                      /*!< RCC secure configuration register                                         */
    __IO uint32_t  PRIVCFGR;                     /*!< RCC privilege configuration register                                      */
} RCC_typedef_t;                                     

#define RCC_BASE (0x44020C00UL)
#define SEC_RCC_BASE (0x54020C00UL)
#define RCC ((__IO RCC_typedef_t *)RCC_BASE)
#define SEC_RCC ((__IO RCC_typedef_t *)SEC_RCC_BASE)

typedef enum {
    RCC_PLL1,
    RCC_PLL2,
    RCC_PLL3
} PLL_num_t;

typedef enum{
    PPRE_DIV_1 = 0x0, //actually it is not DIVided if top(2) bit is not set
    PPRE_DIV_2 = 0b100,
    PPRE_DIV_4 = 0b101,
    PPRE_DIV_8 = 0b110,
    PPRE_DIV_16 = 0b111
} PPRE_state_t;

typedef enum{
    HPRE_DIV_1 = 0x0, //actually it is not DIVided if top(15) bit is not set
    HPRE_DIV_2 = 0b1000,
    HPRE_DIV_4 = 0b1001,
    HPRE_DIV_8 = 0b1010,
    HPRE_DIV_16 = 0b1011,
    HPRE_DIV_64 = 0b1100,
    HPRE_DIV_128 = 0b1101,
    HPRE_DIV_256 = 0b1110,
    HPRE_DIV_512 = 0b1111
} HPRE_state_t;

typedef enum{
    SW_HSI = 0x0,
    SW_CSI = 0x1,
    SW_HSE = 0x2,
    SW_PLL1 = 0x3
}SW_state_t;

typedef enum{
    RCC_AHB1_GPDMA1   = 0x1,
    RCC_AHB1_GPDMA2   = 0x2,
    RCC_AHB1_FLITF    = 0x100,
    RCC_AHB1_CRC      = 0x1000,
    RCC_AHB1_CORDIC   = 0x4000,
    RCC_AHB1_FMAC     = 0x8000,
    RCC_AHB1_RAMCFG   = 0x20000,
    RCC_AHB1_TZSC1    = 0x1000000,
    RCC_AHB1_BKPRAM   = 0x10000000,
    RCC_AHB1_DCACHE   = 0x40000000,
    RCC_AHB1_SRAM1    = 0x80000000
}RCC_AHB1_peripherals_t;

typedef enum{
    RCC_AHB2_GPIOA      = 0x1,
    RCC_AHB2_GPIOB      = 0x2,
    RCC_AHB2_GPIOC      = 0x4,
    RCC_AHB2_GPIOD      = 0x8,
    RCC_AHB2_GPIOE      = 0x10,
    RCC_AHB2_GPIOF      = 0x20,
    RCC_AHB2_GPIOG      = 0x40,
    RCC_AHB2_GPIOH      = 0x80,
    RCC_AHB2_GPIOI      = 0x100,
    RCC_AHB2_ADC12      = 0x400,
    RCC_AHB2_DAC12      = 0x800,
    RCC_AHB2_DCMI_PSSI  = 0x1000,
    RCC_AHB2_HASH       = 0x20000,
    RCC_AHB2_RNG        = 0x40000,
    RCC_AHB2_SRAM3      = 0x40000000,
    RCC_AHB2_SRAM2      = 0x80000000
}RCC_AHB2_peripherals_t;

typedef enum{
    RCC_AHB4_SDMMC1   = 0x800,
    RCC_AHB4_FMC      = 0x10000,
    RCC_AHB4_OCTOSPI1 = 0x100000
}RCC_AHB4_peripherals_t;

typedef enum{
    RCC_APB1L_TIM2     = 0x1,
    RCC_APB1L_TIM3     = 0x2,
    RCC_APB1L_TIM4     = 0x4,
    RCC_APB1L_TIM5     = 0x8,
    RCC_APB1L_TIM6     = 0x10,
    RCC_APB1L_TIM7     = 0x20,
    RCC_APB1L_TIM12    = 0x40,
    RCC_APB1L_TIM13    = 0x80,
    RCC_APB1L_TIM14    = 0x100,
    RCC_APB1L_WWDG     = 0x800,
    RCC_APB1L_SPI2     = 0x4000,
    RCC_APB1L_SPI3     = 0x8000,
    RCC_APB1L_USART2   = 0x20000,
    RCC_APB1L_USART3   = 0x40000,
    RCC_APB1L_UART4    = 0x80000,
    RCC_APB1L_UART5    = 0x100000,
    RCC_APB1L_I2C1     = 0x200000,
    RCC_APB1L_I2C2     = 0x400000,
    RCC_APB1L_I3C1     = 0x800000,
    RCC_APB1L_CRS      = 0x1000000,
    RCC_APB1L_USART6   = 0x2000000,
    RCC_APB1L_USART10  = 0x4000000,
    RCC_APB1L_USART11  = 0x8000000,
    RCC_APB1L_CEC      = 0x10000000,
    RCC_APB1L_UART7    = 0x40000000,
    RCC_APB1L_UART8    = 0x80000000,
}RCC_APB1L_peripherals_t;

typedef enum{
    RCC_APB1H_UART9    = 0x1,
    RCC_APB1H_UART12   = 0x2,
    RCC_APB1H_DTS      = 0x8,
    RCC_APB1H_LPTIM2   = 0x20,
    RCC_APB1H_FDCAN12  = 0x200,
    RCC_APB1H_UCPD     = 0x800000,
}RCC_APB1H_peripherals_t;

typedef enum{
    RCC_APB2_TIM1     = 0x800,
    RCC_APB2_SPI1     = 0x1000,
    RCC_APB2_TIM8     = 0x2000,
    RCC_APB2_USART1   = 0x4000,
    RCC_APB2_TIM15    = 0x10000,
    RCC_APB2_TIM16    = 0x20000,
    RCC_APB2_TIM17    = 0x40000,
    RCC_APB2_SPI4     = 0x80000,
    RCC_APB2_SPI6     = 0x100000,
    RCC_APB2_SAI1     = 0x200000,
    RCC_APB2_SAI2     = 0x400000,
    RCC_APB2_USBFS    = 0x1000000,
}RCC_APB2_peripherals_t;

typedef enum{
    RCC_APB3_SBS      = 0x2,
    RCC_APB3_SPI5     = 0x20,
    RCC_APB3_LPUART1  = 0x40,
    RCC_APB3_I2C3     = 0x80,
    RCC_APB3_I2C4     = 0x100,
    RCC_APB3_LPTIM1   = 0x800,
    RCC_APB3_LPTIM3   = 0x1000,
    RCC_APB3_LPTIM4   = 0x2000,
    RCC_APB3_LPTIM5   = 0x4000,
    RCC_APB3_LPTIM6   = 0x8000,
    RCC_APB3_VREF     = 0x100000,
    RCC_APB3_RTCAPB   = 0x200000,
}RCC_APB3_peripherals_t;

typedef enum{
    PLL_FEATURE_SOURCE_NONE     = 0x0,
    PLL_FEATURE_SOURCE_HSI      = 0x1,
    PLL_FEATURE_SOURCE_CSI      = 0x2,
    PLL_FEATURE_SOURCE_HSE      = 0x3,
    PLL_FEATURE_REF_1_2_MHZ     = 0x0, 
    PLL_FEATURE_REF_2_4_MHZ     = 0x4,
    PLL_FEATURE_REF_4_8_MHZ     = 0x8,
    PLL_FEATURE_REF_8_16_MHZ    = 0xC,
    PLL_FEATURE_FRACK_EN        = 0x10,
    PLL_FEATURE_VCO_WIDE        = 0x0,
    PLL_FEATURE_VCO_MED         = 0x20,
    PLL_FEATURE_PDIV_EN         = 0x10000,
    PLL_FEATURE_QDIV_EN         = 0x20000,
    PLL_FEATURE_RDIV_EN         = 0x40000,
}RCC_PLL_features_t;

#define HSEON_MASK      (0x10000U)
#define HSERDY_MASK     (0x20000U)
#define CSION_MASK      (0x100)
#define CSIRDY_MASK     (0x200)
#define HSION_MASK      (0x1U)
#define HSIRDY_MASK     (0x2U)
#define PLL1ON_MASK     (0x1U << 24)
#define PLL1RDY_MASK    (0x1U << 25)

#define SWS_MASK (0x3 << 3)
#define SW_MASK SW_PLL1

#define PLLSRC_MASK (0x1U << 22)
#define PLLN_SET(val)(((val) & 0x1FF))
#define PLLM_SET(val)(((val) & 0x3F) << 8)
#define PLLQ_SET(val)(((val) & 0x3FU) << 16)
#define PLLR_SET(val)(((val) & 0x3FU) << 24)
#define PLLP_SET(val)(((val) & 0x3FU) << 9)


#define HPRE_SET(val)(((val) & 0xFU))
#define HPRE_MASK (0xF)
#define PPRE1_SET(val)(((val) & 0x7)<<4)
#define PPRE2_SET(val)(((val) & 0x7)<<8)
#define PPRE3_SET(val)(((val) & 0x7)<<12)

#define PPRE1_MASK (0x7<<4)
#define PPRE2_MASK (0x7<<8)
#define PPRE3_MASK (0x7<<12)

#define RCC_ADDITIONAL_PRESCALERS_BITMASK(PLLN, PLLP, PLLQ, PLLR)({ \
    _Static_assert(PLLN-1 && PLLN-1 < 0x200 && PLLN-1 > 0x3,"PLLN invalid");\
    _Static_assert(PLLP-1 < 0x40 && PLLP-1 && (PLLP-1) & 0x1 ,"PLLP invalid");\
    _Static_assert(PLLQ-1 < 0x40,"PLLQ invalid");\
    _Static_assert(PLLR-1 < 0x40,"PLLR invalid");\
    (PLLN_SET(PLLN-1) | PLLP_SET(PLLP-1) | PLLQ_SET(PLLQ-1) | PLLR_SET(PLLR-1));\
})

#define RCC_BUS_PRESCALERS_BITMASK(hpre, ppre1, ppre2, ppre3)({ \
    (HPRE_SET(hpre) | PPRE1_SET(ppre1) | PPRE2_SET(ppre2) | PPRE3_SET(ppre3));\
})


ALWAYS_STATIC void rcc_enable_hsi(__IO RCC_typedef_t *rcc) {
    rcc->CR |= HSION_MASK;
    while (!(rcc->CR & HSIRDY_MASK));
}

ALWAYS_STATIC void rcc_enable_hse(__IO RCC_typedef_t *rcc) {
    rcc->CR |= HSEON_MASK;
    while (!(rcc->CR & HSERDY_MASK));
}

ALWAYS_STATIC void rcc_enable_csi(__IO RCC_typedef_t *rcc) {
    rcc->CR |= CSION_MASK;
    while (!(rcc->CR & CSIRDY_MASK));
}

ALWAYS_STATIC void rcc_set_ahb1_clocking(__IO RCC_typedef_t *rcc, RCC_AHB1_peripherals_t ahb1_mask){
    rcc->AHB1ENR = ahb1_mask;
}

ALWAYS_STATIC void rcc_set_ahb2_clocking(__IO RCC_typedef_t *rcc, RCC_AHB2_peripherals_t ahb2_mask){
    rcc->AHB2ENR = ahb2_mask;
}

ALWAYS_STATIC void rcc_set_ahb3_clocking(__IO RCC_typedef_t *rcc, RCC_AHB4_peripherals_t ahb4_mask){
    rcc->AHB4ENR = ahb4_mask;
}

ALWAYS_STATIC void rcc_set_apb1l_clocking(__IO RCC_typedef_t *rcc, RCC_APB1L_peripherals_t apb1l_mask){
    rcc->APB1LENR = apb1l_mask;
}

ALWAYS_STATIC void rcc_set_apb1_clocking(__IO RCC_typedef_t *rcc, RCC_APB1H_peripherals_t apb1h_mask){
    rcc->APB1HENR = apb1h_mask;
}

ALWAYS_STATIC void rcc_set_apb2_clocking(__IO RCC_typedef_t *rcc, RCC_APB2_peripherals_t apb2_mask){
    rcc->APB2ENR = apb2_mask;
}

ALWAYS_STATIC void rcc_set_apb3_clocking(__IO RCC_typedef_t *rcc, RCC_APB3_peripherals_t apb3_mask){
    rcc->APB3ENR = apb3_mask;
}

ALWAYS_STATIC void rcc_bus_prescalers_setup(__IO RCC_typedef_t *rcc,uint32_t prescalers){
    rcc->CFGR1 &= ~(PPRE1_MASK | PPRE2_MASK | PPRE3_MASK | HPRE_MASK);
    rcc->CFGR1 |= prescalers;
}

ALWAYS_STATIC void rcc_pll_setup(__IO RCC_typedef_t *rcc, PLL_num_t num, uint8_t PLLM, RCC_PLL_features_t features, uint32_t prescalers){
   
    if(!(rcc->CR & HSION_MASK) && (features & PLL_FEATURE_SOURCE_HSE) == PLL_FEATURE_SOURCE_HSI){
        rcc_enable_hsi(rcc);
    }
    
    if(!(rcc->CR & HSEON_MASK) && (features & PLL_FEATURE_SOURCE_HSE) == PLL_FEATURE_SOURCE_HSE){
        rcc_enable_hse(rcc);
    }

    if(!(rcc->CR & CSION_MASK) &&  (features & PLL_FEATURE_SOURCE_HSE) == PLL_FEATURE_SOURCE_CSI){
        rcc_enable_csi(rcc);
    }

    if(rcc->CR & PLL1ON_MASK << num){
        rcc->CR &=~(PLL1ON_MASK << num); 
        while (rcc->CR & PLL1RDY_MASK << num);
    }
    
    __IO uint32_t *divr = (&rcc->PLL1DIVR) + (num * 2);
    __IO uint32_t *cfgr = (&rcc->PLL1CFGR) + num ;
    *divr = prescalers;
    *cfgr = PLLM_SET(PLLM) | features;
    rcc->CR |= PLL1ON_MASK << num;
    while (!(rcc->CR & PLL1RDY_MASK << num));
}

ALWAYS_STATIC void rcc_sysclock_set_input(__IO RCC_typedef_t *rcc, SW_state_t input){
    rcc->CFGR1 &= ~(SW_MASK);
    rcc->CFGR1 |= input;
    while ((RCC->CFGR1 & SWS_MASK)  != (uint32_t)(input<<3));// SWS repots same bits as SW just 2 bits farther
}


extern void rcc_default_sysclock_setup(__IO RCC_typedef_t *rcc){

    uint32_t default_prescalers = RCC_ADDITIONAL_PRESCALERS_BITMASK(BAD_PLLN, BAD_PLLP, BAD_PLLQ, BAD_PLLR);
    rcc_pll_setup(rcc,RCC_PLL1,BAD_PLLM,BAD_RCC_DEFAULT_SETTINGS,default_prescalers);
    uint32_t default_bus_prescalers = RCC_BUS_PRESCALERS_BITMASK(BAD_HPRE,BAD_PPRE1,BAD_PPRE2,BAD_PPRE3);
    rcc_bus_prescalers_setup(rcc,default_bus_prescalers);
    rcc_sysclock_set_input(rcc, SW_PLL1);
}

#endif

#endif // BAD_HAL_USE_RCC

#ifdef BAD_HAL_USE_PWR

typedef struct {                                /*!< PWR Structure                                                             */
    __IO uint32_t  PMCR;                         /*!< PWR power mode control register                                           */
    __IO uint32_t  PMSR;                         /*!< PWR status register                                                       */
    uint32_t  RESERVED[2];
    __IO uint32_t  VOSCR;                        /*!< PWR voltage scaling control register                                      */
    __IO uint32_t  VOSSR;                        /*!< PWR voltage scaling status register                                       */
    uint32_t  RESERVED1[2];
    __IO uint32_t  BDCR;                         /*!< PWR Backup domain control register                                        */
    __IO uint32_t  DBPCR;                        /*!< PWR Backup domain control register                                        */
    __IO uint32_t  BDSR;                         /*!< PWR Backup domain status register                                         */
    __IO uint32_t  UCPDR;                        /*!< PWR USB Type-C power delivery register                                    */
    __IO uint32_t  SCCR;                         /*!< PWR supply configuration control register                                 */
    __IO uint32_t  VMCR;                         /*!< PWR voltage monitor control register                                      */
    __IO uint32_t  USBSCR;                       /*!< PWR USB supply control register                                           */
    __IO uint32_t  VMSR;                         /*!< PWR voltage monitor status register                                       */
    __IO uint32_t  WUSCR;                        /*!< PWR wakeup status clear register                                          */
    __IO uint32_t  WUSR;                         /*!< PWR wakeup status register                                                */
    __IO uint32_t  WUCR;                         /*!< PWR wakeup configuration register                                         */
    uint32_t  RESERVED2;
    __IO uint32_t  IORETR;                       /*!< PWR I/O retention register                                                */
    uint32_t  RESERVED3[43];
    __IO uint32_t  SECCFGR;                      /*!< PWR security configuration register                                       */
    __IO uint32_t  PRIVCFGR;                     /*!< PWR privilege configuration register                                      */
} PWR_typedef_t;                                     

typedef enum{
    PWR_VOS3 = 0x0,
    PWR_VOS2 = 0x1,
    PWR_VOS1 = 0x2,
    PWR_VOS0 = 0x3,
}PWR_vos_state_t;

#define PWR_BASE        (0x44020800UL)
#define SEC_PWR_BASE    (0x54020800UL)

#define PWR ((__IO PWR_typedef_t *)PWR_BASE)
#define SEC_PWR ((__IO PWR_typedef_t)SEC_PWR_BASE)

#define PWR_VOSCR_VOS_MASK      (0x3 << 4)
#define PWR_VOSCR_VOS_SET(vos)  ((vos & 0x3) << 4)
#define PWR_VOSSR_ACTVOSRDY     (1 << 13)
#define PWR_VOSSR_ACTVOS_SHIFT  (14)

#ifdef BAD_PWR_IMPLEMENTATION

extern void pwr_setup_vos(__IO PWR_typedef_t *pwr, PWR_vos_state_t vos){
    while(!(pwr->VOSSR & PWR_VOSSR_ACTVOSRDY));
    pwr->VOSCR = PWR_VOSCR_VOS_SET(vos);
    while(!(pwr->VOSSR & PWR_VOSSR_ACTVOSRDY) && (pwr->VOSSR >> PWR_VOSSR_ACTVOS_SHIFT) != vos);
}    

#endif

#endif

#ifdef BAD_HAL_USE_NVIC

typedef enum{
    WWDG_INTR                 = 0,      /*!< Window WatchDog interrupt                                         */
    PVD_AVD_INTR              = 1,      /*!< PVD/AVD through EXTI Line detection Interrupt                     */
    RTC_INTR                  = 2,      /*!< RTC non-secure interrupt                                          */
    RTC_S_INTR                = 3,      /*!< RTC secure interrupt                                              */
    TAMP_INTR                 = 4,      /*!< Tamper global interrupt                                           */
    RAMCFG_INTR               = 5,      /*!< RAMCFG global interrupt                                           */
    FLASH_INTR                = 6,      /*!< FLASH non-secure global interrupt                                 */
    FLASH_S_INTR              = 7,      /*!< FLASH secure global interrupt                                     */
    GTZC_INTR                 = 8,      /*!< Global TrustZone Controller interrupt                             */
    RCC_INTR                  = 9,      /*!< RCC non secure global interrupt                                   */
    RCC_S_INTR                = 10,     /*!< RCC secure global interrupt                                       */
    EXTI0_INTR                = 11,     /*!< EXTI Line0 interrupt                                              */
    EXTI1_INTR                = 12,     /*!< EXTI Line1 interrupt                                              */
    EXTI2_INTR                = 13,     /*!< EXTI Line2 interrupt                                              */
    EXTI3_INTR                = 14,     /*!< EXTI Line3 interrupt                                              */
    EXTI4_INTR                = 15,     /*!< EXTI Line4 interrupt                                              */
    EXTI5_INTR                = 16,     /*!< EXTI Line5 interrupt                                              */
    EXTI6_INTR                = 17,     /*!< EXTI Line6 interrupt                                              */
    EXTI7_INTR                = 18,     /*!< EXTI Line7 interrupt                                              */
    EXTI8_INTR                = 19,     /*!< EXTI Line8 interrupt                                              */
    EXTI9_INTR                = 20,     /*!< EXTI Line9 interrupt                                              */
    EXTI10_INTR               = 21,     /*!< EXTI Line10 interrupt                                             */
    EXTI11_INTR               = 22,     /*!< EXTI Line11 interrupt                                             */
    EXTI12_INTR               = 23,     /*!< EXTI Line12 interrupt                                             */
    EXTI13_INTR               = 24,     /*!< EXTI Line13 interrupt                                             */
    EXTI14_INTR               = 25,     /*!< EXTI Line14 interrupt                                             */
    EXTI15_INTR               = 26,     /*!< EXTI Line15 interrupt                                             */
    GPDMA1_Channel0_INTR      = 27,     /*!< GPDMA1 Channel 0 global interrupt                                 */
    GPDMA1_Channel1_INTR      = 28,     /*!< GPDMA1 Channel 1 global interrupt                                 */
    GPDMA1_Channel2_INTR      = 29,     /*!< GPDMA1 Channel 2 global interrupt                                 */
    GPDMA1_Channel3_INTR      = 30,     /*!< GPDMA1 Channel 3 global interrupt                                 */
    GPDMA1_Channel4_INTR      = 31,     /*!< GPDMA1 Channel 4 global interrupt                                 */
    GPDMA1_Channel5_INTR      = 32,     /*!< GPDMA1 Channel 5 global interrupt                                 */
    GPDMA1_Channel6_INTR      = 33,     /*!< GPDMA1 Channel 6 global interrupt                                 */
    GPDMA1_Channel7_INTR      = 34,     /*!< GPDMA1 Channel 7 global interrupt                                 */
    IWDG_INTR                 = 35,     /*!< IWDG global interrupt                                             */
    ADC1_INTR                 = 37,     /*!< ADC1 global interrupt                                             */
    DAC1_INTR                 = 38,     /*!< DAC1 global interrupt                                             */
    FDCAN1_IT0_INTR           = 39,     /*!< FDCAN1 interrupt 0                                                */
    FDCAN1_IT1_INTR           = 40,     /*!< FDCAN1 interrupt 1                                                */
    TIM1_BRK_INTR             = 41,     /*!< TIM1 Break interrupt                                              */
    TIM1_UP_INTR              = 42,     /*!< TIM1 Update interrupt                                             */
    TIM1_TRG_COM_INTR         = 43,     /*!< TIM1 Trigger and Commutation interrupt                            */
    TIM1_CC_INTR              = 44,     /*!< TIM1 Capture Compare interrupt                                    */
    TIM2_INTR                 = 45,     /*!< TIM2 global interrupt                                             */
    TIM3_INTR                 = 46,     /*!< TIM3 global interrupt                                             */
    TIM4_INTR                 = 47,     /*!< TIM4 global interrupt                                             */
    TIM5_INTR                 = 48,     /*!< TIM5 global interrupt                                             */
    TIM6_INTR                 = 49,     /*!< TIM6 global interrupt                                             */
    TIM7_INTR                 = 50,     /*!< TIM7 global interrupt                                             */
    I2C1_EV_INTR              = 51,     /*!< I2C1 Event interrupt                                              */
    I2C1_ER_INTR              = 52,     /*!< I2C1 Error interrupt                                              */
    I2C2_EV_INTR              = 53,     /*!< I2C2 Event interrupt                                              */
    I2C2_ER_INTR              = 54,     /*!< I2C2 Error interrupt                                              */
    SPI1_INTR                 = 55,     /*!< SPI1 global interrupt                                             */
    SPI2_INTR                 = 56,     /*!< SPI2 global interrupt                                             */
    SPI3_INTR                 = 57,     /*!< SPI3 global interrupt                                             */
    USART1_INTR               = 58,     /*!< USART1 global interrupt                                           */
    USART2_INTR               = 59,     /*!< USART2 global interrupt                                           */
    USART3_INTR               = 60,     /*!< USART3 global interrupt                                           */
    UART4_INTR                = 61,     /*!< UART4 global interrupt                                            */
    UART5_INTR                = 62,     /*!< UART5 global interrupt                                            */
    LPUART1_INTR              = 63,     /*!< LPUART1 global interrupt                                          */
    LPTIM1_INTR               = 64,     /*!< LPTIM1 global interrupt                                           */
    TIM8_BRK_INTR             = 65,     /*!< TIM8 Break interrupt                                              */
    TIM8_UP_INTR              = 66,     /*!< TIM8 Update interrupt                                             */
    TIM8_TRG_COM_INTR         = 67,     /*!< TIM8 Trigger and Commutation interrupt                            */
    TIM8_CC_INTR              = 68,     /*!< TIM8 Capture Compare interrupt                                    */
    ADC2_INTR                 = 69,     /*!< ADC2 global interrupt                                             */
    LPTIM2_INTR               = 70,     /*!< LPTIM2 global interrupt                                           */
    TIM15_INTR                = 71,     /*!< TIM15 global interrupt                                            */
    TIM16_INTR                = 72,     /*!< TIM16 global interrupt                                            */
    TIM17_INTR                = 73,     /*!< TIM17 global interrupt                                            */
    USB_DRD_FS_INTR           = 74,     /*!< USB FS global interrupt                                           */
    CRS_INTR                  = 75,     /*!< CRS global interrupt                                              */
    UCPD1_INTR                = 76,     /*!< UCPD1 global interrupt                                            */
    FMC_INTR                  = 77,     /*!< FMC global interrupt                                              */
    OCTOSPI1_INTR             = 78,     /*!< OctoSPI1 global interrupt                                         */
    SDMMC1_INTR               = 79,     /*!< SDMMC1 global interrupt                                           */
    I2C3_EV_INTR              = 80,     /*!< I2C3 event interrupt                                              */
    I2C3_ER_INTR              = 81,     /*!< I2C3 error interrupt                                              */
    SPI4_INTR                 = 82,     /*!< SPI4 global interrupt                                             */
    SPI5_INTR                 = 83,     /*!< SPI5 global interrupt                                             */
    SPI6_INTR                 = 84,     /*!< SPI6 global interrupt                                             */
    USART6_INTR               = 85,     /*!< USART6 global interrupt                                           */
    USART10_INTR              = 86,     /*!< USART10 global interrupt                                          */
    USART11_INTR              = 87,     /*!< USART11 global interrupt                                          */
    SAI1_INTR                 = 88,     /*!< Serial Audio Interface 1 global interrupt                         */
    SAI2_INTR                 = 89,     /*!< Serial Audio Interface 2 global interrupt                         */
    GPDMA2_Channel0_INTR      = 90,     /*!< GPDMA2 Channel 0 global interrupt                                 */
    GPDMA2_Channel1_INTR      = 91,     /*!< GPDMA2 Channel 1 global interrupt                                 */
    GPDMA2_Channel2_INTR      = 92,     /*!< GPDMA2 Channel 2 global interrupt                                 */
    GPDMA2_Channel3_INTR      = 93,     /*!< GPDMA2 Channel 3 global interrupt                                 */
    GPDMA2_Channel4_INTR      = 94,     /*!< GPDMA2 Channel 4 global interrupt                                 */
    GPDMA2_Channel5_INTR      = 95,     /*!< GPDMA2 Channel 5 global interrupt                                 */
    GPDMA2_Channel6_INTR      = 96,     /*!< GPDMA2 Channel 6 global interrupt                                 */
    GPDMA2_Channel7_INTR      = 97,     /*!< GPDMA2 Channel 7 global interrupt                                 */
    UART7_INTR                = 98,     /*!< UART7 global interrupt                                            */
    UART8_INTR                = 99,     /*!< UART8 global interrupt                                            */
    UART9_INTR                = 100,    /*!< UART9 global interrupt                                            */
    UART12_INTR               = 101,    /*!< UART12 global interrupt                                           */
    FPU_INTR                  = 103,    /*!< FPU global interrupt                                              */
    ICACHE_INTR               = 104,    /*!< Instruction cache global interrupt                                */
    DCACHE1_INTR              = 105,    /*!< Data cache global interrupt                                       */
    DCMI_PSSI_INTR            = 108,    /*!< DCMI/PSSI global interrupt                                        */
    CORDIC_INTR               = 111,    /*!< CORDIC global interrupt                                           */
    FMAC_INTR                 = 112,    /*!< FMAC global interrupt                                             */
    DTS_INTR                  = 113,    /*!< DTS global interrupt                                              */
    RNG_INTR                  = 114,    /*!< RNG global interrupt                                              */
    HASH_INTR                 = 117,    /*!< HASH global interrupt                                             */
    PKA_INTR                  = 118,    /*!< PKA global interrupt                                              */
    CEC_INTR                  = 119,    /*!< CEC-HDMI global interrupt                                         */
    TIM13_INTR                = 121,    /*!< TIM13 global interrupt                                            */
    TIM14_INTR                = 122,    /*!< TIM14 global interrupt                                            */
    I3C1_EV_INTR              = 123,    /*!< I3C1 event interrupt                                              */
    I3C1_ER_INTR              = 124,    /*!< I3C1 error interrupt                                              */
    I2C4_EV_INTR              = 125,    /*!< I2C4 event interrupt                                              */
    I2C4_ER_INTR              = 126,    /*!< I2C4 error interrupt                                              */
    LPTIM3_INTR               = 127,    /*!< LPTIM3 global interrupt                                           */
    LPTIM4_INTR               = 128,    /*!< LPTIM4 global interrupt                                           */
    LPTIM5_INTR               = 129,    /*!< LPTIM5 global interrupt                                           */
    LPTIM6_INTR               = 130,    /*!< LPTIM6 global interrupt                                           */
} NVIC_programmable_intr_t;

typedef enum{
    NVIC_PRIO0 = 0,
    NVIC_PRIO1,
    NVIC_PRIO2,
    NVIC_PRIO3,
    NVIC_PRIO4,
    NVIC_PRIO5,
    NVIC_PRIO6,
    NVIC_PRIO7,
    NVIC_PRIO8,
    NVIC_PRIO9,
    NVIC_PRIO10,
    NVIC_PRIO11,
    NVIC_PRIO12,
    NVIC_PRIO13,
    NVIC_PRIO14,
    NVIC_PRIO15
} NVIC_prio_t;

typedef struct
{
  __IO uint32_t ISER[16U];              /*!< Offset: 0x000 (R/W)  Interrupt Set Enable Register */
        uint32_t RESERVED0[16U];
  __IO uint32_t ICER[16U];              /*!< Offset: 0x080 (R/W)  Interrupt Clear Enable Register */
        uint32_t RSERVED1[16U];
  __IO uint32_t ISPR[16U];              /*!< Offset: 0x100 (R/W)  Interrupt Set Pending Register */
        uint32_t RESERVED2[16U];
  __IO uint32_t ICPR[16U];              /*!< Offset: 0x180 (R/W)  Interrupt Clear Pending Register */
        uint32_t RESERVED3[16U];
  __IO uint32_t IABR[16U];              /*!< Offset: 0x200 (R/W)  Interrupt Active bit Register */
        uint32_t RESERVED4[16U];
  __IO uint32_t ITNS[16U];              /*!< Offset: 0x280 (R/W)  Interrupt Non-Secure State Register */
        uint32_t RESERVED5[16U];
  __IO uint8_t  IPR[496U];              /*!< Offset: 0x300 (R/W)  Interrupt Priority Register (8Bit wide) */
        uint32_t RESERVED6[580U];
  __IO  uint32_t STIR;                   /*!< Offset: 0xE00 ( /W)  Software Trigger Interrupt Register */
}  NVIC_typedef_t;

#define NVIC_BASE (0xE000E100UL)

#define NVIC ((NVIC_typedef_t*) NVIC_BASE)


ALWAYS_STATIC void nvic_enable_interrupt(NVIC_programmable_intr_t intrnum)
{
    uint8_t ISER_idx = intrnum >> 5; //deside the register by dividing by 32 
    uint32_t ISER_intr_mask = 1 << (intrnum & 0x1F); // the remainder will be the bit number to which we should write
    OPT_BARRIER;
    NVIC->ISER[ISER_idx] = ISER_intr_mask;
    DSB;
    OPT_BARRIER;
}

ALWAYS_STATIC void nvic_disable_interrupt(NVIC_programmable_intr_t intrnum)
{
    uint8_t ICER_idx = intrnum >> 5;
    uint32_t ICER_intr_mask = 1 << (intrnum & 0x1F);
    OPT_BARRIER;
    NVIC->ICER[ICER_idx] = ICER_intr_mask;
    DSB;
    OPT_BARRIER;
} 

ALWAYS_STATIC void nvic_set_interrupt_priority(NVIC_programmable_intr_t intrnum, NVIC_prio_t prio){
    NVIC->IP[intrnum] = prio << 4;
    DSB;
    OPT_BARRIER;
}

#endif // BAD_HAL_USE_NVIC

//Systick
#ifdef BAD_HAL_USE_SYSTICK

typedef struct {
  __IO uint32_t CTRL;                   
  __IO uint32_t LOAD;                   
  __IO uint32_t VAL;                    
  __IO uint32_t CALIB;                  
} Systick_typedef_t;

#define SYSTICK_BASE (0xE000E010UL)

#define SYSTICK ((Systick_typedef_t *)SYSTICK_BASE)


#define SysTick_CTRL_ENABLE 0x1                                           
typedef enum{
    SYSTICK_FEATURE_TICK_INTERRUPT = 0x2, 
    SYSTICK_FEATURE_CLOCK_SOURCE = 0x4,
}SYSTICK_features_t;

ALWAYS_STATIC void systick_setup(uint32_t reload_value,SYSTICK_features_t features){
    SYSTICK->LOAD = reload_value-1;
    SYSTICK->CTRL = features;
    SYSTICK->VAL = 0;
}

ALWAYS_STATIC void systick_enable(){
    SYSTICK->CTRL |= SysTick_CTRL_ENABLE;
}

ALWAYS_STATIC void systick_disable(){
    SYSTICK->CTRL &= ~(SysTick_CTRL_ENABLE);
}

#endif // BAD_HAL_USE_SYSTICK


//Peripherals
//USART
#ifdef BAD_HAL_USE_USART

#ifndef BAD_USART_DEF
#ifdef BAD_USART_STATIC
    #define BAD_USART_DEF ALWAYS_STATIC
#else
    #define BAD_USART_DEF extern
#endif
#endif

typedef struct
{
    __IO uint32_t CR1;         /*!< USART Control register 1,                 Address offset: 0x00  */
    __IO uint32_t CR2;         /*!< USART Control register 2,                 Address offset: 0x04  */
    __IO uint32_t CR3;         /*!< USART Control register 3,                 Address offset: 0x08  */
    __IO uint32_t BRR;         /*!< USART Baud rate register,                 Address offset: 0x0C  */
    __IO uint32_t GTPR;        /*!< USART Guard time and prescaler register,  Address offset: 0x10  */
    __IO uint32_t RTOR;        /*!< USART Receiver Time Out register,         Address offset: 0x14  */
    __IO uint32_t RQR;         /*!< USART Request register,                   Address offset: 0x18  */
    __IO uint32_t ISR;         /*!< USART Interrupt and status register,      Address offset: 0x1C  */
    __IO uint32_t ICR;         /*!< USART Interrupt flag Clear register,      Address offset: 0x20  */
    __IO uint32_t RDR;         /*!< USART Receive Data register,              Address offset: 0x24  */
    __IO uint32_t TDR;         /*!< USART Transmit Data register,             Address offset: 0x28  */
    __IO uint32_t PRESC;       /*!< USART Prescaler register,                 Address offset: 0x2C  */
} USART_typedef_t;
// Compile time divisor calculation

#define USART1_BASE         (0x40013800UL)
#define SEC_USART1_BASE     (0x50013800UL)

#define USART1  ((__IO USART_typedef_t *)USART1_BASE)
#define SEC_USART1  ((__IO USART_typedef_t *)SEC_USART1_BASE)

typedef enum{
    USART_IDLEIE = 0x10,
    USART_RXNEIE = 0x20,
    USART_TCIE = 0x40,
    USART_TXEIE = 0x80,
    USART_PEIE = 0x100
}USART_interrupt_flags_t;


#define USART_CALCULATE_BRR(baud,clock) \
    (((uint16_t)((float)clock/(16*baud)) << 4) | \
     (uint8_t)(((((float)clock/(16*baud)) - (uint16_t)((float)clock/(16*baud)))*16) + 0.5f))


#define USART_BRR_115200 USART_CALCULATE_BRR(115200UL,CLOCK_SPEED)
#define USART_BRR_9600 USART_CALCULATE_BRR(9600UL,CLOCK_SPEED)
#define USART_CR1_USART_ENABLE 0x1
typedef enum{
    USART_SR_PE     = 0x1,
    USART_SR_FE     = 0x2,
    USART_SR_NE     = 0x4,
    USART_SR_ORE    = 0x8,
    USART_SR_IDLE   = 0x10,
    USART_SR_RXNE   = 0x20,
    USART_SR_TC     = 0x40,
    USART_SR_TXE    = 0x80,
    USART_SR_LBDF   = 0x100,
    USART_SR_CTSIF  = 0x200,
    USART_SR_CTS    = 0x400
}USART_SR_flags_t;

typedef enum{
    USART_FEATURE_RECIEVE_EN = 0x4,
    USART_FEATURE_TRANSMIT_EN = 0x8,
    USART_FEATURE_PARITY_EVEN = 0x0,
    USART_FEATURE_PARITY_ODD = 0x200,
    USART_FEATURE_PARITY_OFF = 0x0,
    USART_FEATURE_PARITY_ON = 0x400,
    USART_FEATURE_WAKE_IDLE = 0x0,
    USART_FEATURE_WAKE_ADDR_MART =0x800,  
    USART_FEATURE_8BIT_WORD = 0x0,
    USART_FEATURE_9BIT_WORD = 0x1000,
    USART_FEATURE_OVERSAMPLING_16 =0x0, 
    USART_FEATURE_OVERSAMPLING_8 = 0x8000
}USART_feature_t;

typedef enum{
    USART_MISC_DMA_RECIEVE = 0x40, 
    USART_MISC_DMA_TRANSMIT = 0x80
}USART_misc_t;


ALWAYS_STATIC void uart_enable_misc(__IO USART_typedef_t * USART , USART_misc_t misc){
    USART->CR3 |= misc;
}

ALWAYS_STATIC void uart_disable_misc(__IO USART_typedef_t * USART ,USART_misc_t misc){
    USART->CR3 &= ~(misc);
}

ALWAYS_STATIC void uart_enable_interrupts(__IO USART_typedef_t * USART,USART_interrupt_flags_t interrupts){
    USART->CR1 |= interrupts;
}

ALWAYS_STATIC void uart_disable_interrupts(__IO USART_typedef_t * USART,USART_interrupt_flags_t interrupts){
    USART->CR1 &= ~(interrupts);
}
BAD_USART_DEF void uart_enable(__IO USART_typedef_t* USART);
BAD_USART_DEF void uart_disable(__IO USART_typedef_t * USART);
BAD_USART_DEF void uart_putchar_polling(__IO USART_typedef_t*,char);
BAD_USART_DEF char uart_getchar_polling(__IO USART_typedef_t*);
BAD_USART_DEF void uart_setup(__IO USART_typedef_t * USART,
    uint16_t BRR,
    USART_feature_t features,
    USART_misc_t misc,
    USART_interrupt_flags_t interrupt);
BAD_USART_DEF void uart_send_str_polling(__IO USART_typedef_t* USART ,const char* str);
BAD_USART_DEF void uart_send_hex_32bit(__IO USART_typedef_t* USART,uint32_t value);
BAD_USART_DEF void uart_send_dec_unsigned_32bit(__IO USART_typedef_t *USART ,uint32_t value);

#ifdef BAD_USART_IMPLEMENTATION

BAD_USART_DEF void uart_enable(__IO USART_typedef_t* USART){
    while (!(USART->ISR & USART_SR_TC));
    USART->CR1 |= USART_CR1_USART_ENABLE;
}
BAD_USART_DEF void uart_disable(__IO USART_typedef_t * USART) {
    while (!(USART->ISR & USART_SR_TC));
    USART->CR1 &= ~USART_CR1_USART_ENABLE;
}

BAD_USART_DEF void uart_putchar_polling(__IO USART_typedef_t* USART,char ch){
    while (!(USART->ISR & USART_SR_TXE)); 
    USART->TDR = ch;
}

BAD_USART_DEF char uart_getchar_polling(__IO USART_typedef_t* USART){
    while(!(USART->ISR & USART_SR_RXNE));
    return (char)USART->RDR;
}

BAD_USART_DEF void uart_setup(__IO USART_typedef_t * USART,
    uint16_t BRR,
    USART_feature_t features,
    USART_misc_t misc,
    USART_interrupt_flags_t interrupts)
{
    USART->CR1 = features| interrupts;
    USART->BRR = BRR;
    USART->CR2 = 0; //unsupported for now
    USART->CR3 = misc;
}
BAD_USART_DEF void uart_send_str_polling(__IO USART_typedef_t* USART ,const char* str){
    while(*str){
        uart_putchar_polling(USART,*str);
        str++;
    }   
}

BAD_USART_DEF void uart_send_hex_32bit(__IO USART_typedef_t* USART,uint32_t value){
    const char lookup[] ="0123456789ABCDEF";
    
    for (uint8_t i = 0; i < 8 ;i++){
        uint32_t idx = (value >>28) & 0xF ;
        char c = lookup[idx];
        uart_putchar_polling(USART,c);
        value <<= 4;
    }
    uart_send_str_polling(USART, "\r\n");
}

BAD_USART_DEF void uart_send_dec_unsigned_32bit(__IO USART_typedef_t *USART ,uint32_t value){
    char buff[11];
    char *write = buff+11;
    *--write = 0;

    do{ 
        *--write = (value%10)+'0';
        value/=10;
    }
    while (value!=0);

    uart_send_str_polling(USART, write);
    uart_send_str_polling(USART, "\r\n");
}
#endif

#endif // BAD_HAL_USE_USART

#ifdef BAD_HAL_USE_GPIO

typedef struct
{
    __IO uint32_t MODER;       /*!< GPIO port mode register,               Address offset: 0x00      */
    __IO uint32_t OTYPER;      /*!< GPIO port output type register,        Address offset: 0x04      */
    __IO uint32_t OSPEEDR;     /*!< GPIO port output speed register,       Address offset: 0x08      */
    __IO uint32_t PUPDR;       /*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
    __IO uint32_t IDR;         /*!< GPIO port input data register,         Address offset: 0x10      */
    __IO uint32_t ODR;         /*!< GPIO port output data register,        Address offset: 0x14      */
    __IO uint32_t BSRR;        /*!< GPIO port bit set/reset  register,     Address offset: 0x18      */
    __IO uint32_t LCKR;        /*!< GPIO port configuration lock register, Address offset: 0x1C      */
    __IO uint32_t AFR[2];      /*!< GPIO alternate function registers,     Address offset: 0x20-0x24 */
    __IO uint32_t BRR;         /*!< GPIO Bit Reset register,               Address offset: 0x28      */
    __IO uint32_t HSLVR;       /*!< GPIO high-speed low voltage register,  Address offset: 0x2C      */
    __IO uint32_t SECCFGR;     /*!< GPIO secure configuration register,    Address offset: 0x30      */
} GPIO_typedef_t;

#define GPIOA_BASE      (0x42020000UL)
#define SEC_GPIOA_BASE  (0x52020000UL)
#define GPIOA ((__IO GPIO_typedef_t*)GPIOA_BASE)
#define SEC_GPIOA ((__IO GPIO_typedef_t*)SEC_GPIOA_BASE)

#define GPIOB_BASE      (0x42020400UL)
#define SEC_GPIOB_BASE  (0x52020400UL)
#define GPIOB ((__IO GPIO_typedef_t*)GPIOB_BASE)
#define SEC_GPIOB ((__IO GPIO_typedef_t*)SEC_GPIOB_BASE)

#define GPIOC_BASE      (0x42020800UL)
#define SEC_GPIOC_BASE  (0x52020800UL)
#define GPIOC ((__IO GPIO_typedef_t*)GPIOC_BASE)
#define SEC_GPIOC ((__IO GPIO_typedef_t*)SEC_GPIOC_BASE)

#define BSSRx_BR(x)     (1<<(x + 16))
#define BSSRx_BS(x)     (1 << x)

ALWAYS_STATIC void io_pin_set(__IO GPIO_typedef_t *GPIO, uint8_t pin_num){
    GPIO->BSRR = BSSRx_BS(pin_num);
}

ALWAYS_STATIC void io_pin_reset(__IO GPIO_typedef_t *GPIO, uint8_t pin_num){
    GPIO->BSRR = BSSRx_BR(pin_num);
}

ALWAYS_STATIC void io_pin_toggle(__IO GPIO_typedef_t *GPIO,uint8_t pin_num){
    GPIO->ODR ^= 1 << pin_num;
}

#ifdef BAD_GPIO_IMPLEMENTATION
#define MODERx_MASK(x)  (0x3<<(x * 2))
#define MODERx_SET(val,x)((val & 0x3) <<(x * 2))
#define OSPEEDRx_SET(val,x) ((val & 0x3)<<(x * 2))
#define OSPEEDRx_MASK(x) (0x3<<(x * 2))
#define PUPDRx_SET(val,x) ((val & 0x3)<<(x * 2))
#define PUPDRx_MASK(x) (0x3<<(x * 2))
#define OTx_MASK(x) (1 << x )//1 bit field 0-push pull 1-open drain
#define ODRx_MASK(x)    (1 << x )
#define OTYPERx_SET(val,x) (val << x)

typedef enum {
    MODER_reset_input = 0b00,
    MODER_output = 0b01,
    MODER_af = 0b10,
    MODER_analog = 0b11
}MODERx_states_t;

typedef enum {
    OSPEEDR_low_speed = 0b00,
    OSPEEDR_medium_speed = 0b01,
    OSPEEDR_fast_speed = 0b10,
    OSPEEDR_high_speed = 0b11
}OSPEEDRx_states_t;

#define OSPEEDRx_SET(val,x) ((val & 0x3)<<(x * 2))
#define OSPEEDRx_MASK(x) (0x3<<(x * 2))

typedef enum {
    PUPDR_no_pull = 0b00,
    PUPDR_pullup = 0b01,
    PUPDR_pulldown = 0b10,
    PUPDR_reserved = 0b11
}PUPDRx_states_t;


typedef enum {
    OTYPR_push_pull = 0,
    OTYPR_open_drain = 1
}OTYPRx_state_t;

extern void io_setup_pin(__IO GPIO_typedef_t *GPIO, uint8_t pin_num, MODERx_states_t mode, uint8_t af, OSPEEDRx_states_t speed, PUPDRx_states_t pull, OTYPRx_state_t type){
    GPIO->MODER &=~(MODERx_MASK(pin_num));
    GPIO->MODER |= MODERx_SET(mode, pin_num);
    if (mode == MODER_af) {
        GPIO->AFR[pin_num >> 3] &=  ~(0xFU << (pin_num & 0x7) * 4);
        GPIO->AFR[pin_num >> 3] |= (af & 0xF) << ((pin_num & 0x7)  * 4);
    }
    GPIO->OTYPER &= ~(OTx_MASK(pin_num));
    GPIO->OTYPER |= OTYPERx_SET(type, pin_num);
    GPIO->OSPEEDR &= ~(OSPEEDRx_MASK(pin_num));
    GPIO->OSPEEDR |= OSPEEDRx_SET(speed,pin_num);
    GPIO->PUPDR &= ~PUPDRx_MASK(pin_num);
    GPIO->PUPDR |= PUPDRx_SET(pull,pin_num);
}

#endif

#endif //BAD_HAL_USE_GPIO

//Interrupts
//Hardfault interrupt
//HardFault handler with optional UART logging.
#ifdef BAD_HARDFAULT_ISR_IMPLEMENTATION

#ifdef BAD_HARDFAULT_USE_UART

#ifndef FAULT_LOG_UART
#define FAULT_LOG_UART USART1
#endif

#define FAULT_LOG_UART_SETTINGS (USART_FEATURE_TRANSMIT_EN)

#endif

void __attribute__((naked)) isr_hardfault(){ 
    __asm volatile(
        "cpsid i        \n"
        "tst lr,#4      \n"
        "ite eq         \n"
        "mrseq r0,msp   \n"
        "mrsne r0,psp   \n"
        "b hardfault_c  \n"
    );
}

void __attribute__((used)) hardfault_c(uint32_t* stack){
    volatile uint32_t r0  = stack[0];
    volatile uint32_t r1  = stack[1];
    volatile uint32_t r2  = stack[2];
    volatile uint32_t r3  = stack[3];
    volatile uint32_t r12 = stack[4];
    volatile uint32_t lr  = stack[5]; 
    volatile uint32_t pc  = stack[6];  
    volatile uint32_t psr = stack[7];
    volatile uint32_t hfsr = SCB->HFSR;
    volatile uint32_t cfsr = SCB->CFSR;
    volatile uint32_t mmfar = SCB->MMFAR;
    volatile uint32_t bfar = SCB->BFAR;
    volatile uint32_t afsr = SCB->AFSR;
    volatile uint32_t dfsr = SCB->DFSR;
#ifdef BAD_HARDFAULT_USE_UART
    uart_disable(FAULT_LOG_UART);
    uart_setup(FAULT_LOG_UART,USART_BRR_9600,FAULT_LOG_UART_SETTINGS,0,0);
    uart_enable(FAULT_LOG_UART);
    uart_send_str_polling(FAULT_LOG_UART,"HARDFAULT\r\n");
    uart_send_str_polling(FAULT_LOG_UART, "R0 = ");
    uart_send_hex_32bit(FAULT_LOG_UART, r0);
 
    uart_send_str_polling(FAULT_LOG_UART, "R1 = ");
    uart_send_hex_32bit(FAULT_LOG_UART, r1);

    uart_send_str_polling(FAULT_LOG_UART, "R2 = ");
    uart_send_hex_32bit(FAULT_LOG_UART, r2);

    uart_send_str_polling(FAULT_LOG_UART, "R3 = ");
    uart_send_hex_32bit(FAULT_LOG_UART, r3);

    uart_send_str_polling(FAULT_LOG_UART, "R12 = ");
    uart_send_hex_32bit(FAULT_LOG_UART, r12);

    uart_send_str_polling(FAULT_LOG_UART, "LR = ");
    uart_send_hex_32bit(FAULT_LOG_UART, lr);

    uart_send_str_polling(FAULT_LOG_UART, "!!PC = ");
    uart_send_hex_32bit(FAULT_LOG_UART, pc&~(0x1));

    uart_send_str_polling(FAULT_LOG_UART, "xPSR =  ");
    uart_send_hex_32bit(FAULT_LOG_UART, psr);

    uart_send_str_polling(FAULT_LOG_UART, "SP = ");
    uart_send_hex_32bit(FAULT_LOG_UART, (uint32_t)stack);



    uart_send_str_polling(FAULT_LOG_UART, "CFSR = ");
    uart_send_hex_32bit(FAULT_LOG_UART, cfsr);

    uart_send_str_polling(FAULT_LOG_UART, "HFSR = ");
    uart_send_hex_32bit(FAULT_LOG_UART, hfsr);

    uart_send_str_polling(FAULT_LOG_UART, "DFSR = ");
    uart_send_hex_32bit(FAULT_LOG_UART, dfsr);

    uart_send_str_polling(FAULT_LOG_UART, "MMFAR = ");
    uart_send_hex_32bit(FAULT_LOG_UART, mmfar);

    uart_send_str_polling(FAULT_LOG_UART,"BFAR = " );
    uart_send_hex_32bit(FAULT_LOG_UART, bfar);

    uart_send_str_polling(FAULT_LOG_UART, "AFSR = ");
    uart_send_hex_32bit(FAULT_LOG_UART,afsr );
#endif
    while (1) {
    
    }
}


#endif

//Systick
#ifdef  BAD_SYSTICK_SYSTICK_ISR_IMPLEMENTATION

void systick_usr();

STRONG_ISR(systick_isr){
    systick_usr();
}

#endif

//USART interrupts
#ifdef BAD_USART_USART1_ISR_IMPLEMENTATION
#ifdef BAD_USART_USART1_USE_RXNE
void usart1_rx_isr(char);
#endif

STRONG_ISR(usart1_isr){
    if(USART1->SR & RXNE_MASK){
#ifdef BAD_USART_USART1_USE_RXNE
        usart1_rx_isr(USART1->DR);
#endif
    }
}
#endif

#endif // !BAD_HAL_H
