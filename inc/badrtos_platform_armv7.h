/* date = September 20th 2026 1:30 am */
#ifndef BAD_RTOS_PLATFORM_ARMV7_H
#define BAD_RTOS_PLATFORM_ARMV7_H

#ifndef BAD_RTOS_H
# error "Platform should not be included independently"
#endif

#define BAD_RTOS_ASM_LOAD_PSPLIM

//SCB
typedef struct
{
    volatile u32 CPUID;                  
    volatile u32 ICSR;                   
    volatile u32 VTOR;                   
    volatile u32 AIRCR;                  
    volatile u32 SCR;                    
    volatile u32 CCR;                    
    volatile u8  SHP[12];               
    volatile u32 SHCSR;                  
    volatile u32 CFSR;                   
    volatile u32 HFSR;                   
    volatile u32 DFSR;                   
    volatile u32 MMFAR;                  
    volatile u32 BFAR;                   
    volatile u32 AFSR;                   
    volatile u32 PFR[2];                
    volatile u32 DFR;                    
    volatile u32 ADR;                    
    volatile u32 MMFR[4];               
    volatile u32 ISAR[5];               
    u32 RESERVED0[5];
    volatile u32 CPACR;                  
} bad_scb_typedef_t;

typedef enum
{
    BAD_SCB_MEMORY_MANAGEMENT_INTR = 0,
    BAD_SCB_BUS_FAULT_INTR=1,
    BAD_SCB_USAGE_FAULT_INTR=2,
    BAD_SCB_SVC_INTR = 7,
    BAD_SCB_DEBUG_MONITOR_INTR = 8,
    BAD_SCB_PENDSV_INTR = 10,
    BAD_SCB_SYSTICK_INTR = 11
} bad_scb_core_interrupt_t;

typedef enum 
{
    BAD_SCB_PRIO0 = 0,
    BAD_SCB_LOWEST_PRIO = ((1 << BAD_RTOS_PRIO_BITS) - 1),
} bad_scb_interrupt_priority_t;

typedef enum
{
    BAD_SCB_FPU_NO_ACCESS = 0,
    BAD_SCB_FPU_PRIV_ACCESS = 5,
    BAD_SCB_FPU_FULL_ACCESS = 15,
} bad_scb_fpu_permission_t;

#define BAD_SCB ((bad_scb_typedef_t *) 0xE000ED00UL)

#define BAD_SCB_ICSR_PENDSVSET                  (0x1U << 28U) 

#define BAD_SCB_CPACR_FPU_SHIFT                 20U
#define BAD_SCB_CPACR_FPU_MASK                  (0xFU << BAD_SCB_CPACR_FPU_SHIFT)

static inline void __scb_trigger_pendsv()
{
    BAD_SCB->ICSR = BAD_SCB_ICSR_PENDSVSET;
}

static inline void __scb_set_core_interrupt_priority(bad_scb_core_interrupt_t intr, bad_scb_interrupt_priority_t prio)
{
    BAD_SCB->SHP[intr] = prio << (8U - BAD_RTOS_PRIO_BITS);
}

static inline void __scb_set_fpu_permission_level(bad_scb_fpu_permission_t perms)
{
    BAD_SCB->CPACR &= ~(BAD_SCB_CPACR_FPU_MASK);
    BAD_SCB->CPACR |= perms << BAD_SCB_CPACR_FPU_SHIFT;
    __dsb();
    __isb();
} 

#ifdef BAD_RTOS_USE_FPU

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
typedef struct {
    volatile u32 TYPE;                   
    volatile u32 CTRL;                   
    volatile u32 RNR;                    
    volatile u32 RBAR;                   
    volatile u32 RASR;                   
    volatile u32 RBAR_A1;                
    volatile u32 RASR_A1;                
    volatile u32 RBAR_A2;                
    volatile u32 RASR_A2;                
    volatile u32 RBAR_A3;                
    volatile u32 RASR_A3;                
} bad_mpu_typedef_t;

typedef enum
{
#define BAD_MPU_TEXSCB_NORMAL_NON_SHAREABLE (0x0)
#define BAD_MPU_TEXSCB_NORMAL_SHAREABLE (0x40000)
    BAD_MPU_TEXSCB_STRONGLY_ORDERED = 0,
    BAD_MPU_TEXSCB_SHARED_DEVICE = 0x10000,
    BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRT = 0x20000,
    BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB = 0x30000,
    BAD_MPU_TEXSCB_NORMAL_NON_CACHEABLE = 0x80000,
    BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE = 0xB0000,
    BAD_MPU_TEXSCB_NON_SHAREABLE_DEVICE = 0x100000
} bad_mpu_texscb_features_t;

typedef enum
{
    BAD_MPU_RASR_AP_PRIV_FAULT_UNPRIV_FAULT = 0,
    BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_FAULT = 0x1000000,
    BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_RO = 0x2000000,
    BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_RW = 0x3000000,
    BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_FAULT = 0x5000000,
    BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_RO = 0x6000000
} bad_mpu_permissions_t;

#define BAD_MPU_BASE (0xE000ED90UL)
#define BAD_MPU ((bad_mpu_typedef_t *)BAD_MPU_BASE)

#define BAD_MPU_CTRL_ENABLE         (0x1)
#define BAD_MPU_CTRL_DEFAULT_MAP    (0x4)
#define BAD_MPU_RBAR_VALID          (1u << 4)
#define BAD_MPU_RBAR_REGION(n)      ((n) & 0x0Fu)
#define BAD_MPU_RBAR_ADDR_MASK      (~0x1Fu)

#define BAD_MPU_RASR_ENABLE         (0x1)
#define BAD_MPU_RASR_XN             (0x10000000)

#define BAD_RTOS_STACK_RASR (BAD_MPU_RASR_ENABLE | (0x4)<<1 | \
BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE | \
BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_FAULT)

#define BAD_RTOS_ASM_SET_RNR

static inline void __mpu_enable_with_default_map()
{
    __dmb();
    BAD_MPU->CTRL = BAD_MPU_CTRL_ENABLE | BAD_MPU_CTRL_DEFAULT_MAP;
    __dsb();
    __isb();
}

static inline u32 __mpu_find_size(u32 bytes, bool round_up)
{
    u32 msb = 31 - __builtin_clz(bytes);
    msb += ((1U << msb) == bytes) * round_up;
    return (msb - 1) << 1;
}

static inline void __mpu_default_init()
{
    //ram region
    BAD_MPU->RNR = 0;
    BAD_MPU->RBAR = BAD_RTOS_RAM_ADDR;
    BAD_MPU->RASR = BAD_MPU_RASR_ENABLE |
        __mpu_find_size(BAD_RTOS_RAM_SIZE,false) |
        BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE |
        BAD_MPU_TEXSCB_NORMAL_SHAREABLE |
        BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_RW;
    
    //flash region
    BAD_MPU->RNR = 5;
    BAD_MPU->RBAR = BAD_RTOS_FLASH_RO_ADDR;
    BAD_MPU->RASR = BAD_MPU_RASR_ENABLE |
        __mpu_find_size(BAD_RTOS_FLASH_RO_SIZE,true) |
        BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB |
        BAD_MPU_TEXSCB_NORMAL_SHAREABLE |
        BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_RO;
    //null adress
    BAD_MPU->RNR = 6;
    BAD_MPU->RBAR = BAD_RTOS_FLASH_RO_ADDR;
    BAD_MPU->RASR = BAD_MPU_RASR_ENABLE |(0x4) << 1 |
        BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB |
        BAD_MPU_TEXSCB_NORMAL_SHAREABLE |
        BAD_MPU_RASR_AP_PRIV_FAULT_UNPRIV_FAULT;
    //kernel data structures
    
    BAD_MPU->RNR = 7;
    BAD_MPU->RBAR = BAD_RTOS_RAM_ADDR;
    BAD_MPU->RASR = __mpu_find_size(&__static_stacks - &__kernel_bss,false)|
        BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_FAULT |
        BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE |
        BAD_MPU_TEXSCB_NORMAL_SHAREABLE |
        BAD_MPU_RASR_XN;
    __mpu_enable_with_default_map();
}

static inline bad_rtos_status_t __mpu_translate_settings(bad_tcb_t *tcb,bad_task_descr_t *descr)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    //Stack region
    u32 addr_cast = (u32)tcb->stack;
    bad_mpu_region_t *stack_region = &tcb->regions[0];
    stack_region->__reg0 = addr_cast | BAD_MPU_RBAR_VALID | BAD_MPU_RBAR_REGION(4);
    stack_region->__reg1 = BAD_RTOS_STACK_RASR;
    
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
                
                if((addr_cast - 1) & addr_cast)
                {
                    ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                    goto exit;
                }
                
                if(user_region->type < BAD_MPU_REGION_MAX)
                {
                    u32 reg0_mask = 0;
                    u32 reg1_mask = 0;
                    u32 priv = user_region->settings & BAD_MPU_PRIV_MASK;
                    
                    if(priv == BAD_MPU_PRIV_FAULT_UNPRIV_FAULT)
                        reg1_mask = BAD_MPU_RASR_AP_PRIV_FAULT_UNPRIV_FAULT;
                    else if(priv == BAD_MPU_PRIV_RW_UNPRIV_FAULT)
                        reg1_mask |= BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_FAULT;
                    else if(priv == BAD_MPU_PRIV_RW_UNPRIV_RW)
                        reg1_mask |= BAD_MPU_RASR_AP_PRIV_RW_UNPRIV_RW;
                    else if(priv == BAD_MPU_PRIV_RO_UNPRIV_FAULT)
                        reg1_mask |= BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_FAULT;
                    else if(priv == BAD_MPU_PRIV_RO_UNPRIV_RO)
                        reg1_mask |= BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_RO;
                    else
                    {
                        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                        goto exit;
                    }
                    
                    u32 sh = user_region->settings & BAD_MPU_SH_MASK;
                    if(user_region->type > BAD_MPU_DEVICE_REGIONS_END){
                        if(sh == BAD_MPU_NON_SHAREABLE)
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_NON_SHAREABLE;
                        else if(sh == BAD_MPU_INNER_SHAREABLE || sh == BAD_MPU_OUTER_SHAREABLE)
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_SHAREABLE;
                        else
                        {
                            ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                            goto exit;
                        }
                    }
                    else
                    {
                        if(sh == BAD_MPU_NON_SHAREABLE)
                            reg1_mask |= BAD_MPU_TEXSCB_NON_SHAREABLE_DEVICE;
                        else if(sh == BAD_MPU_INNER_SHAREABLE || sh == BAD_MPU_OUTER_SHAREABLE)
                            reg1_mask |= BAD_MPU_TEXSCB_SHARED_DEVICE;
                        else
                        {
                            ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                            goto exit;
                        }
                    }
                    
                    if(user_region->settings & BAD_MPU_EXECUTE_NEVER)
                        reg1_mask |= BAD_MPU_RASR_XN;
                    
                    switch(user_region->type)
                    {
                        case BAD_MPU_REGION_NONE:{}break;
                        case BAD_MPU_REGION_MAX:{}break;
                        case BAD_MPU_REGION_DEVICE_GRE:{}break;
                        case BAD_MPU_REGION_DEVICE_NGRE:{}break;
                        
                        case BAD_MPU_REGION_NORMAL_NONCACHEABLE:
                        {
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_NON_CACHEABLE;
                        }break;
                        
                        case BAD_MPU_REGION_NORMAL_NT_CACHEABLE_WB:
                        {
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE;
                        }break;
                        
                        case BAD_MPU_REGION_NORMAL_NT_CACHEABLE_WT:
                        {
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRT;
                        }break;
                        
                        case BAD_MPU_REGION_NORMAL_TR_CACHEABLE_WB:
                        {
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE;
                        }break;
                        
                        case BAD_MPU_REGION_NORMAL_TR_CACHEABLE_WT:
                        {
                            reg1_mask |= BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRT;
                        }break;
                    }
                    
                    reg0_mask |= BAD_MPU_RBAR_VALID | BAD_MPU_RBAR_REGION(i);
                    
                    region->__reg0 = addr_cast | reg0_mask;
                    region->__reg1 = __mpu_find_size(user_region->size,true) | reg1_mask;
                }
                else
                {
                    ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                }
            }
        }
        
        for(; i < 4; i++)
        {
            tcb->regions[i] = (bad_mpu_region_t)
            {
                .__reg0 = BAD_MPU_RBAR_VALID | BAD_MPU_RBAR_REGION(i) 
            };
        }
    }
    
    exit:
    return ret;
}

static inline u32 __mpu_kernel_region_save_unlock()
{
    u32 kernel_rasr = BAD_MPU->RASR;
    BAD_MPU->RASR = kernel_rasr & ~(BAD_MPU_RASR_ENABLE);
    __dsb();
    __isb();
    
    return kernel_rasr;
}

static inline void __mpu_kernel_region_restore_lock(u32 key)
{
    BAD_MPU->RASR = key;
    __dsb();
}

#endif

#endif //BADRTOS_PLATFORM_ARMV7_H
