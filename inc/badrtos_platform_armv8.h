/* date = September 18th 2026 11:44 pm */

#ifndef BAD_RTOS_PLATFORM_CM33_H
#define BAD_RTOS_PLATFORM_CM33_H

#ifndef BAD_RTOS_H
# error "Platform should not be included independently"
#endif

#define BAD_RTOS_ASM_LOAD_PSPLIM "ldr r1,[r2,#4] \n" "msr psplim,r1 \n"

//Taken from core_cm33.h from CMSIS, Apache License, Version 2.0 
typedef struct
{
    volatile u32 CPUID;                
    volatile u32 ICSR;                 
    volatile u32 VTOR;                 
    volatile u32 AIRCR;                
    volatile u32 SCR;                  
    volatile u32 CCR;                  
    volatile u8  SHP[12U];             
    volatile u32 SHCSR;                
    volatile u32 CFSR;                 
    volatile u32 HFSR;                 
    volatile u32 DFSR;                 
    volatile u32 MMFAR;                
    volatile u32 BFAR;                 
    volatile u32 AFSR;                 
    volatile u32 ID_PFR[2U];           
    volatile u32 ID_DFR;               
    volatile u32 ID_AFR;               
    volatile u32 ID_MMFR[4U];          
    volatile u32 ID_ISAR[6U];          
    volatile u32 CLIDR;                
    volatile u32 CTR;                  
    volatile u32 CCSIDR;               
    volatile u32 CSSELR;               
    volatile u32 CPACR;                
    volatile u32 NSACR;                
    u32 RESERVED7[21U];
    volatile u32 SFSR;                 
    volatile u32 SFAR;                 
    u32 RESERVED3[69U];
    volatile  u32 STIR;                
    u32 RESERVED4[15U];
    volatile u32 MVFR0;                
    volatile u32 MVFR1;                
    volatile u32 MVFR2;                
    u32 RESERVED5[1U];
    volatile u32 ICIALLU;              
    u32 RESERVED6[1U];
    volatile u32 ICIMVAU;              
    volatile u32 DCIMVAC;              
    volatile u32 DCISW;                
    volatile u32 DCCMVAU;              
    volatile u32 DCCMVAC;              
    volatile u32 DCCSW;                
    volatile u32 DCCIMVAC;             
    volatile u32 DCCISW;               
    volatile u32 BPIALL;               
} bad_scb_typedef_t;

typedef enum
{
    BAD_SCB_MEMORY_MANAGEMENT_INTR = 0,
    BAD_SCB_BUS_FAULT_INTR = 1,
    BAD_SCB_USAGE_FAULT_INTR = 2,
    BAD_SCB_SVC_INTR = 7,
    BAD_SCB_DEBUG_MONITOR_INTR = 8,
    BAD_SCB_PENDSV_INTR = 10,
    BAD_SCB_SYSTICK_INTR = 11
} bad_scb_core_interrupt_t;

typedef enum 
{
    BAD_SCB_PRIO0 = 0,
    BAD_SCB_LOWEST_PRIO = ((1 << BAD_RTOS_PRIO_BITS) - 1)
} bad_scb_interrupt_priority_t;

typedef enum
{
    BAD_SCB_FPU_NO_ACCESS = 0,
    BAD_SCB_FPU_PRIV_ACCESS = 5,
    BAD_SCB_FPU_FULL_ACCESS = 15,
} bad_scb_fpu_permission_t;

#define BAD_SCB_BASE (0xE000ED00UL)
#define BAD_SCB ((bad_scb_typedef_t *) BAD_SCB_BASE)

#define BAD_SCB_ICSR_PENDSVSET                  (0x1 << 28 ) 

#define BAD_SCB_CPACR_FPU_SHIFT                 20U
#define BAD_SCB_CPACR_FPU_MASK                  (0xF << BAD_SCB_CPACR_FPU_SHIFT)

static inline void __scb_trigger_pendsv()
{
    BAD_SCB->ICSR = BAD_SCB_ICSR_PENDSVSET;
}

static inline void __scb_set_core_interrupt_priority(bad_scb_core_interrupt_t intr, bad_scb_interrupt_priority_t prio)
{
    BAD_SCB->SHP[intr] = prio << (8 - BAD_RTOS_PRIO_BITS);
}

static inline void __scb_set_fpu_permission_level(bad_scb_fpu_permission_t perms)
{
    BAD_SCB->CPACR &= ~(BAD_SCB_CPACR_FPU_MASK);
    BAD_SCB->CPACR |= perms << BAD_SCB_CPACR_FPU_SHIFT;
    __dsb();
    __isb();
} 

#ifdef BAD_RTOS_USE_FPU
//Taken from core_cm33.h from CMSIS, Apache License, Version 2.0 
typedef struct
{
    volatile u32 FPCCR;
    volatile u32 FPCAR;
    volatile u32 FPDCR;
    volatile u32 MVFR0;
    volatile u32 MVFR1;
} bad_fpu_typedef_t;

#define BAD_FPU_BASE (0xE000EF34UL)
#define BAD_FPU ((volatile bad_fpu_typedef_t *)BAD_FPU_BASE)

typedef enum 
{
    BAD_FPU_FEATURE_DISALOW_UNPRIV_CHANGE = 0,
    BAD_FPU_FEATURE_ALLOW_UNPRIV_CHANGE = 1,
    BAD_FPU_FEATURE_DISABLE_AUTO_STACKING = 0,
    BAD_FPU_FEATURE_ENABLE_AUTO_STACKING = 0x80000000,
    BAD_FPU_FEATURE_DISABLE_LAZY_STACKING = 0,
    BAD_FPU_FEATURE_ENABLE_LAZY_STACKING = 0x40000000
} bad_fpu_features_t;

static inline void __fpu_init(bad_fpu_features_t features)
{
    BAD_FPU->FPCCR = features;
    __dsb();
    __isb();
}

#define BAD_RTOS_FPU_SETTINGS (BAD_FPU_FEATURE_ENABLE_LAZY_STACKING|BAD_FPU_FEATURE_ENABLE_AUTO_STACKING)
#endif

#ifdef BAD_RTOS_USE_MPU
//Taken from core_cm33.h from CMSIS, Apache License, Version 2.0 
typedef struct
{
    volatile u32 TYPE;                   
    volatile u32 CTRL;                   
    volatile u32 RNR;                    
    volatile u32 RBAR;                   
    volatile u32 RLAR;                   
    volatile u32 RBAR_A1;                
    volatile u32 RLAR_A1;                
    volatile u32 RBAR_A2;                
    volatile u32 RLAR_A2;                
    volatile u32 RBAR_A3;                
    volatile u32 RLAR_A3;                
    u32 RESERVED0[1];
    volatile u32 MAIR[2];
} bad_mpu_typedef_t;

typedef enum
{
    BAD_MPU_MAIR_DEVICE_NGNRNE = 0x0,
    BAD_MPU_MAIR_DEVICE_NGNRE = 0x4,
    BAD_MPU_MAIR_DEVICE_NGRE = 0x8,
    BAD_MPU_MAIR_DEVICE_GRE = 0xC
} bad_mpu_mair_device_states_t;

typedef enum
{
#define BAD_MPU_MAIR_READ_ALLOCATE  (0x1)
#define BAD_MPU_MAIR_WRITE_ALLOCATE (0x2)
    BAD_MPU_MAIR_NORMAL_WT_TRANSIENT_RA_WA = 0x3,
    BAD_MPU_MAIR_NORMAL_WT_TRANSIENT_RA_WN = 0x2,
    BAD_MPU_MAIR_NORMAL_WT_TRANSIENT_RN_WA = 0x1,
    
    BAD_MPU_MAIR_NORMAL_NON_CACHEABLE = 0x4,
    
    BAD_MPU_MAIR_NORMAL_WB_TRANSIENT_RN_WN = 0x4,
    BAD_MPU_MAIR_NORMAL_WB_TRANSIENT_RA_WA = 0x4 | BAD_MPU_MAIR_WRITE_ALLOCATE | BAD_MPU_MAIR_READ_ALLOCATE,
    BAD_MPU_MAIR_NORMAL_WB_TRANSIENT_RA_WN = 0x4 | BAD_MPU_MAIR_READ_ALLOCATE,
    BAD_MPU_MAIR_NORMAL_WB_TRANSIENT_RN_WA = 0x4 | BAD_MPU_MAIR_WRITE_ALLOCATE,
    
    BAD_MPU_MAIR_NORMAL_WT_NON_TRANSIENT_RN_WN = 0x8,
    BAD_MPU_MAIR_NORMAL_WT_NON_TRANSIENT_RA_WA = 0x8 | BAD_MPU_MAIR_WRITE_ALLOCATE | BAD_MPU_MAIR_READ_ALLOCATE,
    BAD_MPU_MAIR_NORMAL_WT_NON_TRANSIENT_RA_WN = 0x8 | BAD_MPU_MAIR_READ_ALLOCATE,
    BAD_MPU_MAIR_NORMAL_WT_NON_TRANSIENT_RN_WA = 0x8 | BAD_MPU_MAIR_WRITE_ALLOCATE,
    
    BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RN_WN = 0xC,
    BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RA_WA = 0xC | BAD_MPU_MAIR_WRITE_ALLOCATE | BAD_MPU_MAIR_READ_ALLOCATE,
    BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RA_WN = 0xC | BAD_MPU_MAIR_READ_ALLOCATE,
    BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RN_WA = 0xC | BAD_MPU_MAIR_WRITE_ALLOCATE
        
} bad_mpu_mair_normal_states_t;

typedef enum
{
    BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_FAULT    = 0x0,
    BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW       = 0x2,
    BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_FAULT    = 0x4,
    BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO       = 0x6
} bad_mpu_ap_states_t;

typedef enum
{
    BAD_MPU_RBAR_SH_NON_SHAREABLE   = 0x0,
    BAD_MPU_RBAR_SH_OUTER_SHAREABLE = 0x8,
    BAD_MPU_RBAR_SH_INNER_SHAREABLE = 0x10
} bad_mpu_sh_states_t;

#define BAD_MPU_BASE (0xE000ED90UL)
#define BAD_MPU ((bad_mpu_typedef_t *)BAD_MPU_BASE)

#define BAD_MPU_CTRL_ENABLE (0x1)
#define BAD_MPU_CTRL_DEFAULT_MAP (0x4)
#define BAD_MPU_MAIR_SET_REGION(settings,idx) ((settings) << ((idx) * 8))
#define BAD_MPU_MAIR_NORMAL_SETTING(outer,inner) (((outer) << 4)|(inner))

#define BAD_RTOS_DEVICE_GRE_MAIR_IDX               (0)
#define BAD_RTOS_DEVICE_NGRE_MAIR_IDX              (1)
#define BAD_RTOS_NORMAL_NON_CACHEABLE              (2)
#define BAD_RTOS_NORMAL_NT_CACHEABLE_WB_MAIR_IDX   (3)
#define BAD_RTOS_NORMAL_NT_CACHEABLE_WT_MAIR_IDX   (4)
#define BAD_RTOS_NORMAL_TR_CACHEABLE_WB_MAIR_IDX   (5)
#define BAD_RTOS_NORMAL_TR_CACHEABLE_WT_MAIR_IDX   (6)

#define BAD_MPU_RBAR_XN (0x1)
#define BAD_MPU_RLAR_EN (0x1)

#define BAD_MPU_RLAR_SET_MAIR_IDX(idx) ((idx) << 1)

#define BAD_RTOS_STACK_RBAR     (BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW | BAD_MPU_RBAR_XN)
#define BAD_RTOS_STACK_RLAR     (BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_NT_CACHEABLE_WB_MAIR_IDX) | BAD_MPU_RLAR_EN)

#define BAD_RTOS_ASM_SET_RNR "mov r1, #0 \n" "str r1,[r12] \n"

static inline void __mpu_enable_with_default_map()
{
    __dmb();
    BAD_MPU->CTRL = BAD_MPU_CTRL_ENABLE | BAD_MPU_CTRL_DEFAULT_MAP;
    __dsb();
    __isb();
}

#define BAD_RTOS_MAIR0_SETTINGS \
BAD_MPU_MAIR_SET_REGION(\
BAD_MPU_MAIR_DEVICE_GRE,BAD_RTOS_DEVICE_GRE_MAIR_IDX) | \
BAD_MPU_MAIR_SET_REGION(\
BAD_MPU_MAIR_DEVICE_NGRE,BAD_RTOS_DEVICE_NGRE_MAIR_IDX) | \
BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
BAD_MPU_MAIR_NORMAL_NON_CACHEABLE,\
BAD_MPU_MAIR_NORMAL_NON_CACHEABLE), BAD_RTOS_NORMAL_NON_CACHEABLE) |\
BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RA_WA,\
BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RA_WA), BAD_RTOS_NORMAL_NT_CACHEABLE_WB_MAIR_IDX)

#define BAD_RTOS_MAIR1_SETTINGS \
BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
BAD_MPU_MAIR_NORMAL_WT_NON_TRANSIENT_RA_WA,\
BAD_MPU_MAIR_NORMAL_WT_NON_TRANSIENT_RA_WA), BAD_RTOS_NORMAL_NT_CACHEABLE_WT_MAIR_IDX - 4)|\
BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
BAD_MPU_MAIR_NORMAL_WB_TRANSIENT_RA_WA,\
BAD_MPU_MAIR_NORMAL_WB_TRANSIENT_RA_WA), BAD_RTOS_NORMAL_TR_CACHEABLE_WB_MAIR_IDX - 4)|\
BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
BAD_MPU_MAIR_NORMAL_WT_TRANSIENT_RA_WA,\
BAD_MPU_MAIR_NORMAL_WT_TRANSIENT_RA_WA), BAD_RTOS_NORMAL_TR_CACHEABLE_WT_MAIR_IDX - 4)

static inline void __mpu_default_init()
{
    BAD_MPU->MAIR[0] = BAD_RTOS_MAIR0_SETTINGS;
    BAD_MPU->MAIR[1] = BAD_RTOS_MAIR1_SETTINGS;
    
    // 0x0, force a fault through overlapping regions lol
    BAD_MPU->RNR = 4;
    BAD_MPU->RBAR = (BAD_RTOS_FLASH_RO_ADDR) | BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO;
    BAD_MPU->RLAR = (BAD_RTOS_FLASH_RO_ADDR) | BAD_MPU_RLAR_EN| BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_NT_CACHEABLE_WB_MAIR_IDX);
    
    //global data
    BAD_MPU->RNR = 5;
    BAD_MPU->RBAR = (u32)(&__heap) | BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW;
    BAD_MPU->RLAR = ((u32)(&__dma_buffs) - 32) | BAD_MPU_RLAR_EN | BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_NON_CACHEABLE);
    
    //flash region
    BAD_MPU->RNR = 6;
    BAD_MPU->RBAR = (BAD_RTOS_FLASH_RO_ADDR) | BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO;
    BAD_MPU->RLAR = (BAD_RTOS_FLASH_RO_ADDR + BAD_RTOS_FLASH_RO_SIZE - 32) | BAD_MPU_RLAR_EN| BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_NT_CACHEABLE_WB_MAIR_IDX);
    
    //kernel data 
    BAD_MPU->RNR = 7;
    BAD_MPU->RBAR = (u32)(&__kernel_bss) | BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_FAULT;
    BAD_MPU->RLAR = ((u32)(&__ekernel_data) - 32) | BAD_MPU_RLAR_EN | BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_NT_CACHEABLE_WB_MAIR_IDX);
    
    __mpu_enable_with_default_map();
}

static inline bad_rtos_status_t __mpu_translate_settings(bad_tcb_t *tcb, bad_task_descr_t *descr)
{
    
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    //Stack region
    if(!tcb->dyn_stack)
    {
        u32 addr_cast = (u32)tcb->stack;
        bad_mpu_region_t *stack_region = &tcb->regions[0];
        stack_region->__reg0 = addr_cast | BAD_RTOS_STACK_RBAR;
        stack_region->__reg1 = (addr_cast + tcb->stack_size) | BAD_RTOS_STACK_RLAR;
    }
    { //User regions
        u32 i = 1;
        if(descr && descr->regions)
        {
            for(; i < 4; i++)
            {
                bad_mpu_region_t *region = &tcb->regions[i];
                const bad_mpu_user_region_t *user_region = &descr->regions[i - 1];
                u32 addr_cast = (u32)user_region->addr;
                
                if(user_region->type == BAD_MPU_REGION_NONE)
                    break;
                
                if(user_region->size % 32 || addr_cast % 32)
                {
                    ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                    goto exit;
                }
                
                if(user_region->type < BAD_MPU_REGION_MAX)
                {
                    u32 reg0_mask = 0;
                    u32 reg1_mask = 0;
                    u32 priv = user_region->settings & BAD_MPU_PRIV_MASK;
                    
                    if(priv == BAD_MPU_PRIV_RW_UNPRIV_FAULT)
                        reg0_mask |= BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_FAULT;
                    else if(priv == BAD_MPU_PRIV_RW_UNPRIV_RW)
                        reg0_mask |= BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW;
                    else if(priv == BAD_MPU_PRIV_RO_UNPRIV_FAULT)
                        reg0_mask |= BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_FAULT;
                    else if(priv == BAD_MPU_PRIV_RO_UNPRIV_RO)
                        reg0_mask |= BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO;
                    else
                    {
                        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                        goto exit;
                    }
                    
                    u32 sh = user_region->settings & BAD_MPU_SH_MASK;
                    
                    if(sh == BAD_MPU_NON_SHAREABLE)
                        reg0_mask |= BAD_MPU_RBAR_SH_NON_SHAREABLE;
                    else if(sh == BAD_MPU_INNER_SHAREABLE)
                        reg0_mask |= BAD_MPU_RBAR_SH_INNER_SHAREABLE;
                    else if(sh == BAD_MPU_OUTER_SHAREABLE)
                        reg0_mask |= BAD_MPU_RBAR_SH_OUTER_SHAREABLE;
                    else
                    {
                        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                        goto exit;
                    }
                    
                    if(user_region->settings & BAD_MPU_EXECUTE_NEVER)
                        reg0_mask |= BAD_MPU_RBAR_XN;
                    
                    u32 mair_idx = user_region->type - 1;
                    reg1_mask = BAD_MPU_RLAR_SET_MAIR_IDX(mair_idx) | BAD_MPU_RLAR_EN;
                    
                    region->__reg0 = addr_cast | reg0_mask;
                    region->__reg1 = (addr_cast + user_region->size - 32) | reg1_mask;
                }
                else
                {
                    ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                }
            }
        }
        
        for(; i < 4; i++)
        {
            tcb->regions[i] = (bad_mpu_region_t){0};
        }
    }
    
    exit:
    return ret;
}

static inline u32 __mpu_kernel_region_save_unlock()
{
    u32 kernel_rlar = BAD_MPU->RLAR;
    BAD_MPU->RLAR = kernel_rlar & ~(BAD_MPU_RLAR_EN);
    __dsb();
    __isb();
    
    return kernel_rlar;
}

static inline void __mpu_kernel_region_restore_lock(u32 key)
{
    BAD_MPU->RLAR = key;
    __dsb();
}
#endif

#endif //BADRTOS_PLATFORM_CM33_H
