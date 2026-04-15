/**
 * @file bad_rtos.h
 * @brief Header only rtos implementation
 *
 *
 *
 * Usage:
 *  - Include this file and define BAD_RTOS_IMPLEMENTATION in **one**
 *    C file
 *  - Change the config to your liking
 *  - Define the bad_user_setup function and all the perliminary setup there, like task creation 
 *  - Call bad_rtos_start to start rtos operation
 * Notes:
 *  - Depends on the startup code , linker file and some headers from my codebase, but can easly be decoupled from them
 *    if the mpu is not used, mpu setup assumes kernel data to be at the start of ram , and the end of it 
 *    to be alligned by  32, which ensures allignment by 32 for static and dynamic stacks
 *  - ! Kernel syscall interrupt priority is 1, Pendsv and systick priorities are 15 so for user interrupts use 
 *    priorities 2-14
 *  - !! If the task uses FPU make sure the stack size can accomodate additional 33 registers 
 */
#pragma once
#ifndef BAD_RTOS_CORE
#define BAD_RTOS_CORE

#include <stdint.h>

#define BAD_RTOS_FLASH_SIZE (0x100000)

//CONFIG
//uncoment those to enable desired functionality
#define BAD_RTOS_USE_KHEAP      //kernel heap
//#define KMIN_ORDER 5          //kernel heap minimal order of allocation (size = 1 << MIN_ORDER = 32)
//#define KMAX_ORDER 12         //kernel heap maximum order of allocation (heap_size) (size = 1 << MIN_ORDER = 4096)
#define BAD_RTOS_USE_UHEAP      //user heap
//#define UMIN_ORDER 5          //user heap minimal order of allocation (size = 1 << MIN_ORDER = 32)
//#define UMAX_ORDER 12         //user heap maximum order of allocation (heap size) (size = 1 << MIN_ORDER = 4096)
#define BAD_RTOS_USE_MUTEX      //mutexes
#define BAD_RTOS_USE_MSGQ       // message queues
#define BAD_RTOS_USE_SEMAPHORE  //semaphores
#define BAD_RTOS_USE_MPU        //mpu
#define BAD_RTOS_USE_FPU        //fpu
#define BAD_RTOS_FPU_DEFAULT_SETTING //use default settings for the fpu (lazy + auto stacking enabled),if custom settings used - comment this and enable lazy stacking

#define BAD_RTOS_MAX_TASKS 32   //maximum number of running tasks, number of user priorities = BAD_RTOS_MAX_TASKS-2, with idle task running at BAD_RTOS_MAX_TASKS-1
#define BAD_RTOS_SVC_PRIO 1     // Keep this higher than any ISR priority that interacts with the API and tick and Pendsv priorities

//set those to whatever name your hal sets them as WEAK
#define BAD_RTOS_SVC_HANDLER_NAME svc_isr
#define BAD_RTOS_PENDSV_HANDLER_NAME pendsv_isr
//dont forget to setup the timer and set its interrupt priority to 15
#define BAD_RTOS_TICK_HANDLER_NAME systick_isr

#if BAD_RTOS_MAX_TASKS < 2
    #error "Number of tasks must be > 1 to accomodate for idle task"
#elif BAD_RTOS_MAX_TASKS > 32
    #error "Number of tasks must be <=32"
#endif

// asm helpers implemented in the asm portion of this file grep for "ASM stuff" to find it
static inline uint32_t __attribute__((always_inline)) __modify_basepri(uint32_t basepri);
static inline void __attribute__((always_inline)) __restore_basepri(uint32_t basepri);

// critical section helpers using basepri
#define CRITICAL_MASK ((BAD_RTOS_SVC_PRIO + 1) << 4)
#define CONTEX_SWITCH_BARIER_STORE uint32_t context_basepri = 0;
#define CONTEX_SWITCH_BARIER context_basepri = __modify_basepri(15<<4)
#define CONTEX_SWITCH_BARIER_RELEASE __restore_basepri(context_basepri)
#define CRITICAL_SECTION_STORE uint32_t critical_basepri = 0;
#define CRITICAL_SECTION critical_basepri = __modify_basepri(CRITICAL_MASK)
#define CRITICAL_SECTION_END __restore_basepri(critical_basepri)
// error codes 
typedef enum {
    BAD_RTOS_STATUS_ALLOC_FAIL = 0, // to return as null ptr
    BAD_RTOS_STATUS_OK,
    BAD_RTOS_STATUS_BAD_PARAMETERS,
    BAD_RTOS_STATUS_NOT_BLOCKED,
    BAD_RTOS_STATUS_NOT_DELAYED,
    BAD_RTOS_STATUS_HANDLE_INVALID,
    BAD_RTOS_STATUS_WOULD_BLOCK,
    BAD_RTOS_STATUS_CANT_YEILD,
    BAD_RTOS_STATUS_CANT_FINISH,
    BAD_RTOS_STATUS_TIMEOUT,
    BAD_RTOS_STATUS_WRONG_Q,
    BAD_RTOS_STATUS_NOT_OWNER,
    BAD_RTOS_STATUS_RECURSIVE_TAKE,
    BAD_RTOS_STATUS_WOKEN,
    BAD_RTOS_STATUS_DELETED,
    BAD_RTOS_STATUS_NOT_INITIALISED,
    BAD_RTOS_STATUS_WRONG_CONTEXT
}bad_rtos_status_t;
// helper enum to discriminate the position of the tcb in a queue
// the logic behind it is
// member enum = head enum + 1 
// this allows to easily check if a tcb is a member of the respective queue or not
typedef enum {
    BAD_RTOS_MISC_RUNNING,
    BAD_RTOS_MISC_READYQ_MEMBER,
    BAD_RTOS_MISC_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER
} bad_rtos_misc_t;
// helper enum for software timer queue
// folows the same logic as the enum above
typedef enum{
    BAD_RTOS_MISC_NOT_DELAYED,
    BAD_RTOS_MISC_DELAYQ_MEMBER
}bad_rtos_delayq_misc_t;

#ifdef BAD_RTOS_USE_MPU
//mpu region struct, stores the precomputed rasr and the adress of the region
typedef struct {
    uintptr_t rbar;
    uintptr_t rlar;
}mpu_region_t;
#endif

typedef uint32_t bad_task_handle_t;

typedef void (*taskptr)(void * par) ;
typedef void (*cbptr)(bad_task_handle_t handle,void * par);


typedef struct bad_link_node{
    struct bad_link_node *prev;
    struct bad_link_node *next;
} bad_link_node_t;

// main fat struct of the program
typedef struct bad_tcb{
    // stack pointer, doesnt really reflect the actual one when running, actual one is + 32
    // (due to registers stacked by hardware), used only to save it for a context switch     
    uint32_t * volatile sp;
    //stack base
    uint32_t *stack;
    uint32_t stack_size;
    taskptr entry;
    //callback for delays
    cbptr cbptr;
    //args for the callback
    void* args;
    bad_link_node_t qnode;
    bad_link_node_t delaynode;
    uint32_t ticks_to_change;   
    volatile uint32_t counter;
#if defined (BAD_RTOS_USE_MPU)
    const mpu_region_t *regions;
#endif
    bad_rtos_misc_t misc;
    bad_rtos_delayq_misc_t delayq_misc;
    uint16_t generation; //for handles
#ifdef BAD_RTOS_USE_KHEAP
    uint8_t dyn_stack;
#endif
    //execution priority, follows nvic logic : lower number is higher priority
    uint8_t base_priority;
    uint8_t raised_priority;
#ifdef BAD_RTOS_USE_MUTEX
    uint8_t mutex_count;
#endif
}bad_tcb_t;

//task decription for task creation
typedef struct{
    uint32_t stack_size;
    uint32_t* stack;
    taskptr entry;
    void* args;
    uint32_t ticks_to_change;
#ifdef BAD_RTOS_USE_MPU
    const mpu_region_t *regions;
#endif
    uint8_t base_priority;
}bad_task_descr_t;


#ifdef BAD_RTOS_USE_MUTEX
typedef struct bad_mutex{
    bad_tcb_t *owner;
    bad_link_node_t blockedq;
} bad_mutex_t ;
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
typedef struct bad_sem{
    uint16_t counter;
    uint16_t init_flag;
    bad_link_node_t blockedq;
} bad_sem_t;

typedef struct bad_nbsem{
    volatile uint32_t init_flag;
    volatile uint32_t counter;
} bad_nbsem_t;

#endif

#ifdef BAD_RTOS_USE_MSGQ
typedef struct {
    uint32_t signal;
    void *args;
} bad_msg_block_t;

typedef struct {
    bad_msg_block_t *msgs;
    volatile uint32_t head;
    volatile uint32_t tail;
    uint32_t capacity;
} bad_msgq_t;

typedef enum {
    BAD_QUEUE_EMPTY = 0,
    BAD_QUEUE_OK,
    BAD_QUEUE_BAD_PARAMETERS, //same value as BAD_RTOS_STATUS_BAD_PARAMETERS
    BAD_QUEUE_OVERRUN,
    BAD_QUEUE_NOT_INITIALISED,
}bad_queue_status_t;
#endif


#ifdef BAD_RTOS_USE_MPU
/** 
 * Macros for MPU region definition 
 * Usage example:
 * START_TASK_MPU_REGIONS_DEFINITIONS(task1)
 *      DEFINE_PERIPH_ACCESS_REGION(task1,USART1_BASE, sizeof(USART_typedef_t))
 * END_TASK_MPU_REGIONS(task1)
 * Then in task creation:
 *    bad_task_descr_t task1_descr = {
 *      .stack = 0,
 *      .stack_size = TASK1_STACK_SIZE,
 *      .dyn_stack = 1,
 *      .entry = task1,
 *      .regions = task1_regions,
 *      .region_count = MPU_REGIONS_SIZE(task1),
 *      .ticks_to_change = 500,
 *      .base_priority = TASK2_PRIORITY
 *  };
 */

typedef enum{
    BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_FAULT    = 0x0,
    BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW       = 0x2,
    BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_FAULT    = 0x4,
    BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO       = 0x6
}bad_mpu_ap_states_t;

typedef enum{
    BAD_MPU_RBAR_SH_NON_SHAREABLE   = 0x0,
    BAD_MPU_RBAR_SH_OUTER_SHAREABLE = 0x8,
    BAD_MPU_RBAR_SH_INNER_SHAREABLE = 0x10
}bad_mpu_sh_states_t;

#define BAD_RTOS_NORMAL_MAIR_IDX    (0)
#define BAD_RTOS_DEVICE_MAIR_IDX    (1)
#define BAD_RTOS_DMA_BUFF_MAIR_IDX  (2)

#define BAD_MPU_RBAR_XN (0x1)
#define BAD_MPU_RLAR_EN (0x1)

#define BAD_MPU_RLAR_SET_MAIR_IDX(idx) ((idx) << 1)

#define BAD_RTOS_STACK_RBAR     (BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW | BAD_MPU_RBAR_XN)
#define BAD_RTOS_STACK_RLAR     (BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_MAIR_IDX) | BAD_MPU_RLAR_EN)

#define BAD_RTOS_DMA_BUFF_RBAR     (BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW | BAD_MPU_RBAR_XN)
#define BAD_RTOS_DMA_BUFF_RLAR     (BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_DMA_BUFF_MAIR_IDX)|\
                                        BAD_MPU_RBAR_SH_OUTER_SHAREABLE|BAD_MPU_RLAR_EN)

#define START_TASK_MPU_REGIONS_DEFINITIONS(name)\
static const  mpu_region_t name##_regions[4]={

#define DEFINE_GENERIC_REGION(address, size,mair_idx, share ,xn, ap)\
{\
    .rbar = (uintptr_t)(address) | (xn)| (share)| (ap) ,\
    .rlar = ((uintptr_t)((address) + (size) ) | BAD_MPU_RLAR_SET_MAIR_IDX(idx) |  BAD_MPU_RLAR_EN\
},
#define DEFINE_PERIPH_ACCESS_REGION(address, size) \
{\
    .rbar = (uintptr_t)(address) | BAD_MPU_RBAR_XN | BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW ,\
    .rlar = ((uintptr_t)((address) + (size)) & ~(0x1F) | BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_DEVICE_MAIR_IDX) |  BAD_MPU_RLAR_EN\
},

#define DEFINE_STATIC_STACK_REGION(address, size) \
{\
    .rbar = (uintptr_t)(address + BAD_RTOS_STACK_RBAR) ,\
    .rlar = (uintptr_t)((address) + (size) - 32 + BAD_RTOS_STACK_RLAR)\
},

#define DEFINE_DMA_BUFF_REGION(address,size) \
{\
    .rbar = (uintptr_t)(address + BAD_RTOS_DMA_BUFF_RBAR) ,\
    .rlar = (uintptr_t)((address) + (size) - 32 + BAD_RTOS_DMA_BUFF_RLAR)\
},

#define END_TASK_MPU_REGIONS(name) \
};

#define DEFINE_DMA_BUFF(name,size) \
_Static_assert((size) % 32 == 0,"DMA buffers should be multiples of 32");\
static uint8_t __attribute__((section(".dma_buffs"))) name[(size)];

#endif 
//Macro for static stack definition
#ifdef BAD_RTOS_USE_MPU
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert(size % 32 == 0,"Stack sizes must be multiples of 32");\
_Static_assert(size >= 128,"Stacks must be at least 128 bytes to accomodate exception stacked registers and stack cannary");\
static uint8_t task_name##_stack[size] __attribute__((section(".static_stacks")));
#else 
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert(size % 8 == 0,"Stack sizes must be multiples of 8");\
_Static_assert(size >= 64,"Stacks must be at least 64 bytes to accomodate exception stacked registers");\
static uint8_t task_name##_stack[size] __attribute__((section(".static_stacks")));
#endif 

// PUBLIC API**********************************************
/**
 * \b BAD_TASK_HANDLE_IS_VALID
 *  Public macro to check the validity of the task handle
 *  
 *  @param[in] bad_task_handle_t task handle
 *
 *  @retval 1 valid
 *  @retval 0 invalid
 */
#define BAD_TASK_HANDLE_IS_VALID(handle) ({ \
    _Static_assert(__builtin_types_compatible_p(typeof(handle), (bad_task_handle_t){0}), "Type of variable differs from bad_task_handle_t");\
    (((handle) & 0xFFFF) != 0xFFFF);\
})

/**
 * \b BAD_TASK_HANDLE_INVALID_GET_ERROR
 *  Public macro to get error from invalid taskhandle
 *  
 *  @param[in] bad_task_handle_t invalid task handle
 *
 *  @retval BAD_RTOS_STATUS_BAD_PARAMETERS on bad configurations
 *  @retval BAD_RTOS_STATUS_ALLOC_FAIL on allocation falure
 *
 */

#define BAD_TASK_HANDLE_INVALID_GET_ERROR(handle) ((handle) >> 16)


/**
 * \b task_make
 *
 * Public SVC (svc 0x11) call that calls internal function __task_make
 * Allocates a tcb object, initialses it with parameters passed using a descriptor (bad_task_descr_t)
 *
 * Created task can preempt the current running task 
 *
 * Allocates the stack if needed using kernel buddy heap
 *
 * This function can be called from interrupt context.
 *
 * @param[in] bad_task_descr_t * Pointer to a descriptor object
 *
 * @retval  bad_task_handle_t Task handle
 * @retval  invalid bad_task_handle_t on falure 
 */
extern bad_task_handle_t task_make(bad_task_descr_t *descr);

/**
 * \b task_delay
 *
 * Public SVC (svc 0xF0) call that calls internal function __task_delay
 * Delays the caller task (current running task) by a number of tick provided in a parameter
 * 
 * Enqueues current task into a delta list using the second set of tcb pointers 
 * Then switches context to the highest priority task ready
 *
 * The delay has a jitter of 1 tick i.e task delayed for N ticks can wake up after N-1 ticks if it requests delay 
 * at the end of the current tick, so its advised to use blocking api for more reliable task synchronisation
 *
 * Delays can be canceled using task_delay_cancel, which would return BAD_RTOS_STATUS_WOKEN to the specified task using
 * stacked registers
 *
 * Caller can also provide a callback function which will be run when delay finishes with arguments provided 
 * as the third argument. Callback runs with Handler priviledge level, so be cautious with it.
 *
 * task_delay(0) is not supported, use task_yield to try to yield
 *
 * This function cannot be called from interrupt context. Will return BAD_RTOS_WRONG_CONTEXT if done so.
 *
 * @param[in] uint32_t delay in ticks 
 * @param[in] cbptr cb callback to run 
 * @param[in] void* args arguments for the callback
 *
 * @retval  BAD_RTOS_STATUS_OK delay time ran out
 * @retval  BAD_RTOS_STATUS_WOKEN the task was woken by another task or isr
 * @retval  BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr  
 */
extern bad_rtos_status_t task_delay(uint32_t delay, cbptr cb, void *args );
/**
 * \b task_block
 *
 * Public SVC (svc 0xE0) call that calls internal function __task_block
 * Blocks the current task until another task or isr unblocks it 
 *   
 * Enqueues current task into an unordeded kernel list of blocked tasks  
 * Then switches context to the highest priority ready task
 * 
 * Tasks are unblocked using task_unblock() public function
 *
 * This function cannot be called from interrupt context. Will return BAD_RTOS_WRONG_CONTEXT if done so.
 *
 * @retval  BAD_RTOS_STATUS_OK task is successfully blocked
 * @retval  BAD_RTOS_WRONG_CONTEXT the function was called by an isr  
 */
extern bad_rtos_status_t task_block();
/**
 * \b task_unblock
 *
 * Public SVC (svc 0x22) call that calls internal function __task_unblock
 * Unblocks the specifed task and tries to preempt the current one
 *
 * Dequeues the specified task from unordeded kernel list of blocked tasks 
 * If the task is not in blocked list(depending on the misc field) returns BAD_RTOS_STATUS_NOT_BLOCKED
 *
 * Tasks are unblocked using task_unblock() public function
 *
 * This function can be called from interrupt context.
 * @param[in] bad_task_handle_t Task handle
 *
 * @retval  BAD_RTOS_STATUS_OK task is successfully unblocked
 * @retval  BAD_RTOS_STATUS_NOT_BLOCKED the task is not blocked
 * @retval  BAD_RTOS_STATUS_HANDLE_INVALID handle is invalid
 */
extern bad_rtos_status_t task_unblock(bad_task_handle_t task);
/**
 * \b task_yield
 *
 * Public SVC (svc 0xD0) call that calls internal function __task_yield
 * Tries to yield to a same priority task
 *
 * 
 * If succedes enqueues current task into ready queue and yields to the task of the same priority if availible
 * Then switches context 
 *
 * This function cannot be called from interrupt context. Will return BAD_RTOS_WRONG_CONTEXT if done so.
 *
 * @retval  BAD_RTOS_STATUS_OK task successfully yielded
 * @retval  BAD_RTOS_STATUS_CANT_YEILD no task to yield to
 * @retval  BAD_RTOS_WRONG_CONTEXT the function was called by an isr
 */
extern bad_rtos_status_t task_yield();
/**
 * \b task_finish
 *
 * Public SVC (svc 0x40) call that calls internal function __task_finish
 * Finishes the execution of the task, frees the tcb and the stack if it was dynamically allocated
 * 
 * Call this only when every resourse held by task is released
 *
 * If task holds mutexes which is reflected in tcb->mutex_count tries to trap
 *
 * This function cannot be called from interrupt context. Will return BAD_RTOS_WRONG_CONTEXT if done so.
 *
 * @retval  BAD_RTOS_STATUS_CANT_FINISH task still holds mutexes, do not rely on this behavior, this is for debug only
 * @retval  BAD_RTOS_WRONG_CONTEXT the function was called by an isr
 */
extern bad_rtos_status_t task_finish();
/**
 * \b task_delay_cancel
 *
 * Public SVC (svc 0x33) call that calls internal function __task_delay_cancel
 * Wakes the task from delay without running the callback
 *
 * Dequeues the specified task from kernel delay delta list
 * Tries to preempt the currently running task 
 *
 * This function can be called from interrupt context.
 * @param[in] bad_task_handle_t Task handle
 *
 * @retval BAD_RTOS_STATUS_OK tasks delay successfully canceled
 * @retval BAD_RTOS_STATUS_NOT_DELAYED task is not delayed 
 * @retval BAD_RTOS_STATUS_HANDLE_INVALID handle invalid 
 */
extern bad_rtos_status_t task_delay_cancel(bad_task_handle_t task);

#ifdef BAD_RTOS_USE_MUTEX
// Priority inheriting mutex api
/**
 * \b mutex_init
 *
 * Public function to initialise mutex object
 * Zero initialises both fields
 * No need to call this if the mutex is already 0 initialised
 *
 * Masks context switch and systick interrupts
 *
 * This function can be called from interrupt context. But is not reentrant if the object parameter is the same
 * @param[in] bad_mutex_t* Ptr to mutex object to initialise
 *
 * @retval BAD_RTOS_STATUS_OK mutex successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
 */
extern bad_rtos_status_t mutex_init(bad_mutex_t *mut);
/**
 * \b mutex_take
 *
 * Public SVC (svc 0xC0) call that calls internal function __mutex_take
 * Tries to take the mutex
 * If the mutex has no owner then the caller becomes the mutexes owner, increasing his mutex count by 1  
 * If it has an owner the behavior depends on the delay value specified
 *
 * delay = 0 : task is blocked. Task is inserted into mutexes blocking priority queue and 
 * if this tasks priority is higher than the owners priority owner inherits priority of the blocked task
 *
 * delay = -1 : take fails and BAD_RTOS_STATUS_WOULD_BLOCK is returned 
 *
 * delay = N : task tries to acquire mutex for N ticks. Task is inserted into mutexes blocking priority queue and 
 * if this tasks priority is higher than the owners priority owner inherits priority of the blocked task. 
 * If the task doesnt become mutexes owner in N ticks task is removed from mutexes blocking queue and reinserted 
 * into ready queue with BAD_RTOS_STATUS_TIMEOUT code in tasks stacked registers
 *
 * This api cannot be called recursively
 *
 * This function cannot be called from interrupt context.Will return BAD_RTOS_WRONG_CONTEXT if done so.
 *
 * @param[in] bad_mutex_t* Ptr to mutex object to try take  
 * @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
 *
 * @retval BAD_RTOS_STATUS_OK Mutex successfully taken
 * @retval BAD_RTOS_STATUS_RECURSIVE_TAKE the current owner tried to take the mutex again
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
 * @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
 * @retval BAD_RTOS_STATUS_WRONG_CONTEXT function was called from an isr
 */
extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,uint32_t delay);
/**
 * \b mutex_put
 *
 * Public SVC (svc 0xB0) call that calls internal function __mutex_put
 * Tries to put the mutex
 *
 * If the caller is the owner then the highest priority blocked task is woken with BAD_RTOS_STATUS_OK written to its 
 * stacked registers, its callback is canceled and tries to preempt the current running task. 
 * If there is no blocked task mutex becomes free. Previous owners mutex count is decreased
 * by 1 and if it is 0 previous owners priority is reset to base priority
 * 
 * If the caller is not the owner BAD_RTOS_STATUS_NOT_OWNER returned
 *
 *
 * This function cannot be called from interrupt context.Will return BAD_RTOS_WRONG_CONTEXT if done so.
 *
 * @param[in] bad_mutex_t* Ptr to mutex object to try put  
 *
 * @retval BAD_RTOS_STATUS_OK Mutex successfully put
 * @retval BAD_RTOS_STATUS_NOT_OWNER caller is not the owner of this mutex object
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex object is NULL
 * @retval BAD_RTOS_STATUS_WRONG_CONTEXT function was called from an isr
 */
extern bad_rtos_status_t mutex_put(bad_mutex_t *mut);
/**
 * \b mutex_delete
 *
 * Public SVC (svc 0xA0) call that calls internal function __mutex_delete
 * Tries to delete the mutex object, doesnt infuence the underlying memory, just resets the object
 *
 * If the caller is the owner then wakes up all the tasks with BAD_RTOS_STATUS_DELETED written into their 
 * stacked registers 
 * If the caller is not the owner BAD_RTOS_STATUS_NOT_OWNER returned
 *
 *
 * This function cannot be called from interrupt context.Will return BAD_RTOS_WRONG_CONTEXT if done so.
 * @param[in] bad_mutex_t* Ptr to mutex object to try delete  
 *
 * @retval BAD_RTOS_STATUS_OK Mutex successfully deleted
 * @retval BAD_RTOS_STATUS_NOT_OWNER caller is not the owner of this mutex object
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex object is NULL
 */
extern bad_rtos_status_t mutex_delete(bad_mutex_t *mut);
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
// Blocking semaphore api 
/**
 * \b sem_init
 *
 * Public function to initialise semaphore object
 * initialises count field to the specifed count
 *
 * No need to call this if you can use an initiliser like bad_sem_t sem = {.counter = N,.init_flag = 1 }
 *
 * Masks context switch and systick interrupts
 *
 * This function can be called from interrupt context. But is not reentrant if the object parameter is the same
 * @param[in] bad_sem_t* Ptr to semaphore object to initialise
 * @param[in] uint16_t Value to initialise semaphore counter with
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore ptr is null 
 */
extern bad_rtos_status_t sem_init(bad_sem_t *sem,uint16_t reset_value);
/**
 * \b sem_take
 *
 * Public SVC (svc 0x77) call that calls internal function __sem_take
 * Tries to take the semaphore
 * If the semaphores counter is not zero decrements the semaphores counter
 * If the semaphores counter is 0 the behavior depends on the delay value specified
 *
 * delay = 0 : task is blocked. Task is inserted into semaphores blocking priority queue  
 *
 * delay = -1 : take fails and BAD_RTOS_STATUS_WOULD_BLOCK is returned 
 *
 * delay = N : task tries to acquire semaphore for N ticks. Task is inserted into semaphores blocking priority queue.
 * If N ticks passed and task failed to acquire semaphore task is removed from semaphores blocking queue and reinserted 
 * into ready queue with BAD_RTOS_STATUS_TIMEOUT code in tasks stacked registers
 *
 * If the function is called from the isr delay value is ignored and treated as -1
 *
 * This function can be called from interrupt context. 
 *
 * @param[in] bad_sem_t* Ptr to semaphore object to try take  
 * @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
 *
 * @retval BAD_RTOS_STATUS_OK Semaphore successfully taken
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
 * @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
 */
extern bad_rtos_status_t sem_take(bad_sem_t *sem,uint32_t delay);
/**
 * \b sem_put
 *
 * Public SVC (svc 0x88) call that calls internal function __sem_put
 * Tries to put the semaphore
 *
 * If the semaphores counter is 0 and a blocked task exists the highest priority blocked task 
 * is woken with BAD_RTOS_STATUS_OK written to its 
 * stacked registers, its callback is canceled and tries to preempt the current running task. 
 * If there is no blocked task semaphore counter is incremented. 
 *
 *
 * This function can be called from interrupt context. But the current running task can be preempted by 
 * an unblocked task
 * @param[in] bad_sem_t* Ptr to sem object to try put  
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully put
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
 */
extern bad_rtos_status_t sem_put(bad_sem_t *sem);
/**
 * \b sem_delete
 *
 * Public SVC (svc 0x99) call that calls internal function __sem_delete
 * Tries to delete the semaphore object, doesnt infuence the underlying memory, just resets the object
 *
 * Wakes up all the tasks with BAD_RTOS_STATUS_DELETED written into their 
 * stacked registers 
 *
 *
 * This function can be called from interrupt context. But loops over semaphores blocked queue
 * @param[in] bad_sem_t* Ptr to semaphore object to try delete  
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully deleted
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
 */
extern bad_rtos_status_t sem_delete(bad_sem_t *sem);
//Non blocking semaphore api
/**
 * \b nbsem_init
 *
 * Public function to initialise non blocking semaphore object
 * initialises count field to the specifed count
 *
 * No need to call this if you can use an initiliser like bad_nbsem_t sem = {.count = N, .init_flag = 1 }
 *
 * Masks context switch and systick interrupts
 *
 * This function can be called from interrupt context. But is not reentrant if the object parameter is the same
 * @param[in] bad_sem_t* Ptr to semaphore object to initialise
 * @param[in] uint32_t Value to initialise semaphore with
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS nbsemaphore ptr is null
 */
extern bad_rtos_status_t nbsem_init(bad_nbsem_t *sem, uint32_t reset_value);
/**
 * \b nbsem_take
 *
 * Public function that tries to take the nonblocking semaphore 
 * If the semaphores counter is not zero decrements the semaphores counter
 * If the semaphores counter is 0 then BAD_RTOS_STATUS_WOULD_BLOCK returned
 *
 * 
 * into ready queue with BAD_RTOS_STATUS_TIMEOUT code in tasks stacked registers
 *
 * This function can be called from interrupt context. This function is reentrant 
 *
 * @param[in] bad_sem_t* Ptr to semaphore object to try take  
 *
 * @retval BAD_RTOS_STATUS_OK Semaphore successfully taken
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS sem ptr is null
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
 */
extern bad_rtos_status_t nbsem_take(bad_nbsem_t *sem);
/** 
 * \b nbsem_put
 *
 * Public  function that tries to put the nonblocking semaphore
 *
 * If the semaphores counter is less than its recorded count then the counter is incremented
 * Else BAD_RTOS_STATUS_RECURSIVE_PUT returned
 *
 * This function can be called from interrupt context. This function is reentrant
 *
 * @param[in] bad_sem_t* Ptr to sem object to try put  
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully put
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
 */
extern bad_rtos_status_t nbsem_put(bad_nbsem_t *sem);
/**
 * \b sem_delete
 *
 * Public function that tries to delete the semaphore object 
 * Doesnt infuence the underlying memory, just resets the object
 *
 *
 * This function can be called from interrupt context.  
 * @param[in] bad_sem_t* Ptr to semaphore object to try delete  
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully deleted
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
 */
extern bad_rtos_status_t nbsem_delete(bad_nbsem_t *sem);
#endif


#ifdef BAD_RTOS_USE_MSGQ
//Macro for static queue allocation
#define MSGQ_STATIC_INIT(name,size)\
_Static_assert((size & (size-1)) == 0, "queue size must be a power of 2"); \
bad_msg_block_t name##_blocks [size];\
bad_msgq_t name = {.capacity = size,.msgs = name##_blocks};
//Nonblocking MPSC message queue api
#if defined (BAD_RTOS_USE_UHEAP) || defined (BAD_RTOS_USE_KHEAP)
/**
 * \b msgq_init
 *
 * Public function that tries to initialise the message queue object 
 * Allocates the message queue memory using caller provided function
 * Message queue control block should be allocated by the caller beforehand
 *
 * If the queue was allocated using the MSGQ_STATIC_INIT macro this should not be called
 *
 * This function can be called from interrupt context. But it allocates memory so common use is discouraged.
 * This function is not reentrant. And if called concurrently on the same object will leak memory
 * @param[in] bad_msgq_t* Ptr to message queue control block to try initialise  
 * @param[in] uint32_t Size of the queue
 * @param[in] allocator function 
 *
 * @retval BAD_RTOS_STATUS_OK message queue successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS queue ptr is 0 or size of the queue is 0 or not power of 2
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL allocation failed
 */
extern bad_rtos_status_t msgq_init(bad_msgq_t *q ,uint32_t size, void* (* alloc_func)(uint32_t size));
/**
 * \b msgq_deinit
 *
 * Public function that tries to deinitialise the message queue object 
 * Dellocates the message queue memory using caller provided function
 * Message queue control block should be deallocated by the caller afterwards
 *
 * If the queue was allocated using the MSGQ_STATIC_INIT macro this should not be called
 *
 * This function can be called from interrupt context. But it deallocates memory so common use is discouraged
 * This function is not reentrant. And if called concurrently on the same object will double free memory
 * @param[in] bad_msgq_t* Ptr to message queue control block to try deinitialise  
 * @param[in] deallocator function
 *
 * @retval BAD_RTOS_STATUS_OK message queue successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS queue ptr or free_func is 0 or size of the queue is 0 or not power of 2 or
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL allocation failed
 */
extern bad_rtos_status_t msgq_deinit(bad_msgq_t *q,void(* free_func)(void *block,uint32_t size));
#endif
/**
 * \b msgq_pull_msg
 *
 * Public function that tries to pull the message from the message queue by updating tail and writing the message data
 * to a passed message block object
 * 
 * If used to pass messages between the isrs make sure that the produsers have higher priority than the consumer
 * 
 *
 * This function can be called from interrupt context.   
 * @param[in] bad_msgq_t* Ptr to message queue control block to try pull messages from  
 * @param[out] bad_msg_block_t message object allocated by the caller to write data to 
 *
 * @retval BAD_QUEUE_OK message pulled successfully
 * @retval BAD_QUEUE_BAD_PARAMETERS queue ptr or writeback ptr is null
 * @retval BAD_QUEUE_EMPTY queue is empty
 */
extern bad_queue_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback );
/**
 * \b msgq_post_msg
 *
 * Public function that tries to post the message from the message queue by updating head and writing the message data
 * to a passed message block object
 * 
 * If used to pass messages between the isrs make sure that the produsers have higher priority than the consumer
 * 
 *
 * This function can be called from interrupt context.   
 * @param[in] bad_msgq_t* Ptr to message queue control block to try pull messages from  
 * @param[in] uint32_t signal to transmit
 * @param[in] void * args to transmit
 *
 * @retval BAD_QUEUE_OK message posted successfully
 * @retval BAD_QUEUE_BAD_PARAMETERS queue ptr or is null
 * @retval BAD_QUEUE_OVERRUN queue is overrun
 */
extern bad_queue_status_t msgq_post_msg(bad_msgq_t *q, uint32_t signal, void *args);
#endif

#if defined (BAD_RTOS_USE_KHEAP)
/**
 * \b kernel_alloc
 *
 * Public SVC (svc 0x55) call that calls internal function __kernel_alloc
 * Tries to allocate a specifed number of bytes from kernel heap
 *
 * Uses buddy allocator under the hood
 *
 * This function can be called from interrupt context. But since it dynamically allocates memory isr usage is discouraged
 * @param[in] uint32_t size in bytes 
 * 
 * @retval void * to allocated memory
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL (null ptr) allocation failed 
 */
extern void* kernel_alloc(uint32_t size);
/**
 * \b kernel_alloc
 *
 * Public SVC (svc 0x66) call that calls internal function __kernel_free
 * Tries to free a specifed number of bytes allocated from kernel heap
 *
 * Uses buddy allocator under the hood
 *
 * This function can be called from interrupt context. But since it dynamically frees memory isr usage is discouraged
 * @param[in] void * to allocated memory 
 * @param[in] uint32_t size in bytes 
 * 
 */
extern void kernel_free(void *block,uint32_t size);
#endif

#if defined (BAD_RTOS_USE_UHEAP)
/**
 * \b user_alloc
 *
 * Public function that tries to allocate a specifed number of bytes from user heap
 *
 * Uses buddy allocator under the hood
 *
 * This function cannot be called from interrupt context. Will return NULL if done so
 * @param[in] uint32_t size in bytes 
 * 
 * @retval void * to allocated memory
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL (null ptr) allocation failed 
 */
extern void* user_alloc(uint32_t size);
/**
 * \b user_free
 *
 * Public function that tries to free a specifed number of bytes allocated from user heap
 * 
 * Uses buddy allocator under the hood
 *
 * This function cannot be called from interrupt context. Will fail silently if done so
 * @param[in] uint32_t size in bytes 
 * @param[in] void * to allocated memory
 * 
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL (null ptr) allocation failed 
 */
extern void user_free(void *block, uint32_t size);
#endif

//****************************************************************
#ifdef BAD_RTOS_IMPLEMENTATION

#define BAD_RTOS_PRIO_COUNT BAD_RTOS_MAX_TASKS

typedef struct kernel_cb {
    volatile uint32_t ticks;
    bad_tcb_t * volatile curr;
    bad_tcb_t * volatile next;
    bad_link_node_t delayq;
    uint8_t is_running;
    uint32_t ready_bmask;
    bad_link_node_t readyq[BAD_RTOS_PRIO_COUNT];
    bad_link_node_t blockedq;
}bad_kernel_cb_t;

typedef struct bitmask_slab_cb{
    uint32_t free_bitmask;
    bad_tcb_t node_arr[BAD_RTOS_MAX_TASKS];
} tcb_bitmask_slab_t;

typedef enum {
    BAD_SYSTICK_TIMEFRAME_PENDING = 0x1,
    BAD_SYSTICK_DELAY_WAKE_PENDING = 0x2,
    BAD_SYSTICK_BOTH = BAD_SYSTICK_DELAY_WAKE_PENDING|BAD_SYSTICK_TIMEFRAME_PENDING//0x3
}bad_systick_status_t;

static bad_kernel_cb_t __attribute__((section(".kernel_bss"))) kernel_cb;

static tcb_bitmask_slab_t __attribute__((section(".kernel_bss"))) tcbslab;

#if defined (BAD_RTOS_USE_UHEAP) || defined (BAD_RTOS_USE_KHEAP)

typedef struct {
    uint8_t* heap;
    uint32_t heads_bmask;
    uint32_t max_order;
    uint32_t min_order;
    bad_link_node_t* free_list;
    uint32_t* bmask;
}bad_buddy_t;
#define BUDDY_BITMASK_SIZE(max_order,min_order)\
    (((1 << (max_order - min_order))-1)+31) >> 5 // bits required = (2 ^ max_order - min_order) - 1, to get the words divide by 32 and round up 
#endif

#if defined (BAD_RTOS_USE_KHEAP) 

#ifndef KMIN_ORDER
    #define KMIN_ORDER 5
#else 
    #if KMIN_ORDER < 3 
        #error "Minimal order should be >= 3 to store the pointers to the next free block and the prev block "
    #endif
#endif

#ifndef KMAX_ORDER 
    #define KMAX_ORDER 12
#else
    #if KMAX_ORDER < MIN_ORDER
        #error "Its called max order for a reason"
    #endif
#endif

#define KHEAP_SIZE 1<<KMAX_ORDER
#define KFREE_LIST_SIZE (KMAX_ORDER-KMIN_ORDER+1)
    
static uint8_t __attribute__((section(".kheap"))) kheap[KHEAP_SIZE];
static bad_buddy_t __attribute__((section(".kernel_bss")))kernel_buddy;
static bad_link_node_t __attribute__((section(".kernel_bss"))) kfreelist[KFREE_LIST_SIZE];
static uint32_t __attribute__((section(".kernel_bss"))) kbitmask[BUDDY_BITMASK_SIZE(KMAX_ORDER, KMIN_ORDER)]; 
#endif

#if defined (BAD_RTOS_USE_UHEAP)

#ifndef UMIN_ORDER
    #define UMIN_ORDER 5
#else 
    #if UMIN_ORDER < 3 
        #error "Minimal order should be >= 3 to store the pointers to the next free block and the prev block "
    #endif
#endif
#ifndef UMAX_ORDER 
    #define UMAX_ORDER 12
#else
    #if KMAX_ORDER < MIN_ORDER
        #error "Its called max order for a reason"
    #endif
#endif

#define UHEAP_SIZE 1<<UMAX_ORDER
#define UFREE_LIST_SIZE (UMAX_ORDER-UMIN_ORDER+1)

static uint8_t __attribute__((section(".uheap"))) uheap[UHEAP_SIZE];
static bad_buddy_t user_buddy;
static bad_link_node_t ufreelist[UFREE_LIST_SIZE];
static uint32_t ubitmask[BUDDY_BITMASK_SIZE(UMAX_ORDER, UMIN_ORDER)];
#endif

#define IDLE_TASK_PRIO BAD_RTOS_PRIO_COUNT-1
#define IDLE_TASK_STACK_SIZE 128

TASK_STATIC_STACK(idle, IDLE_TASK_STACK_SIZE)
// Prototypes for asm helpers, for implementation look right above svc_c function, or grep for "ASM stuff"
extern void idle_task();
extern void __first_task_start();
static inline uint32_t __attribute__((always_inline)) __ldrex(volatile uint32_t* addr);
static inline uint32_t  __attribute__((always_inline)) __strex(uint32_t val,volatile uint32_t* addr);
static inline void  __attribute__((always_inline)) __clrex();
static inline void __attribute__((always_inline)) __set_control(uint32_t control);
static inline uint32_t __attribute__((always_inline)) __get_control();
static inline void __attribute__((always_inline)) __dmb();
static inline void __attribute__((always_inline)) __dsb();
static inline void __attribute__((always_inline)) __isb();

#define BAD_OPT_BARRIER __asm__ volatile("":::"memory")
#define BAD_RTOS_STATIC static inline

#define BAD_TASK_HANDLE_GEN(gen) ((gen) << 16)
#define BAD_TASK_HANDLE_GET_IDX(handle) ((handle) & 0xFFFF)
#define BAD_TASK_HANDLE_GET_GEN(handle) ((handle) >> 16)
#define BAD_TASK_HANDLE_INVALID_HANDLE(error) ((error) << 16|0xFFFF)

#define BAD_CONTAINER_OF(ptr, type, member) ({ \
    _Static_assert(__builtin_types_compatible_p(typeof(*(ptr)), typeof(((type *)0)->member)), \
                   "Pointer type mismatch in container_of"); \
    ((type *)( (char *)(ptr) - __builtin_offsetof(type, member) ));\
})
//Hardware helpers
//Taken from core_cm33.h from CMSIS, Apache License, Version 2.0 

typedef struct{
    volatile uint32_t CPUID;                /*!< Offset: 0x000 (R/ )  CPUID Base Register */
    volatile uint32_t ICSR;                 /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register */
    volatile uint32_t VTOR;                 /*!< Offset: 0x008 (R/W)  Vector Table Offset Register */
    volatile uint32_t AIRCR;                /*!< Offset: 0x00C (R/W)  Application Interrupt and Reset Control Register */
    volatile uint32_t SCR;                  /*!< Offset: 0x010 (R/W)  System Control Register */
    volatile uint32_t CCR;                  /*!< Offset: 0x014 (R/W)  Configuration Control Register */
    volatile uint8_t  SHP[12U];             /*!< Offset: 0x018 (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15) */
    volatile uint32_t SHCSR;                /*!< Offset: 0x024 (R/W)  System Handler Control and State Register */
    volatile uint32_t CFSR;                 /*!< Offset: 0x028 (R/W)  Configurable Fault Status Register */
    volatile uint32_t HFSR;                 /*!< Offset: 0x02C (R/W)  HardFault Status Register */
    volatile uint32_t DFSR;                 /*!< Offset: 0x030 (R/W)  Debug Fault Status Register */
    volatile uint32_t MMFAR;                /*!< Offset: 0x034 (R/W)  MemManage Fault Address Register */
    volatile uint32_t BFAR;                 /*!< Offset: 0x038 (R/W)  BusFault Address Register */
    volatile uint32_t AFSR;                 /*!< Offset: 0x03C (R/W)  Auxiliary Fault Status Register */
    volatile uint32_t ID_PFR[2U];           /*!< Offset: 0x040 (R/ )  Processor Feature Register */
    volatile uint32_t ID_DFR;               /*!< Offset: 0x048 (R/ )  Debug Feature Register */
    volatile uint32_t ID_AFR;               /*!< Offset: 0x04C (R/ )  Auxiliary Feature Register */
    volatile uint32_t ID_MMFR[4U];          /*!< Offset: 0x050 (R/ )  Memory Model Feature Register */
    volatile uint32_t ID_ISAR[6U];          /*!< Offset: 0x060 (R/ )  Instruction Set Attributes Register */
    volatile uint32_t CLIDR;                /*!< Offset: 0x078 (R/ )  Cache Level ID register */
    volatile uint32_t CTR;                  /*!< Offset: 0x07C (R/ )  Cache Type register */
    volatile uint32_t CCSIDR;               /*!< Offset: 0x080 (R/ )  Cache Size ID Register */
    volatile uint32_t CSSELR;               /*!< Offset: 0x084 (R/W)  Cache Size Selection Register */
    volatile uint32_t CPACR;                /*!< Offset: 0x088 (R/W)  Coprocessor Access Control Register */
    volatile uint32_t NSACR;                /*!< Offset: 0x08C (R/W)  Non-Secure Access Control Register */
    uint32_t RESERVED7[21U];
    volatile uint32_t SFSR;                 /*!< Offset: 0x0E4 (R/W)  Secure Fault Status Register */
    volatile uint32_t SFAR;                 /*!< Offset: 0x0E8 (R/W)  Secure Fault Address Register */
    uint32_t RESERVED3[69U];
    volatile  uint32_t STIR;                /*!< Offset: 0x200 ( /W)  Software Triggered Interrupt Register */
    uint32_t RESERVED4[15U];
    volatile uint32_t MVFR0;                /*!< Offset: 0x240 (R/ )  Media and VFP Feature Register 0 */
    volatile uint32_t MVFR1;                /*!< Offset: 0x244 (R/ )  Media and VFP Feature Register 1 */
    volatile uint32_t MVFR2;                /*!< Offset: 0x248 (R/ )  Media and VFP Feature Register 2 */
    uint32_t RESERVED5[1U];
    volatile uint32_t ICIALLU;              /*!< Offset: 0x250 ( /W)I-Cache Invalidate All to PoU */
    uint32_t RESERVED6[1U];
    volatile uint32_t ICIMVAU;              /*!< Offset: 0x258 ( /W)I-Cache Invalidate by MVA to PoU */
    volatile uint32_t DCIMVAC;              /*!< Offset: 0x25C ( /W)D-Cache Invalidate by MVA to PoC */
    volatile uint32_t DCISW;                /*!< Offset: 0x260 ( /W)D-Cache Invalidate by Set-way */
    volatile uint32_t DCCMVAU;              /*!< Offset: 0x264 ( /W)D-Cache Clean by MVA to PoU */
    volatile uint32_t DCCMVAC;              /*!< Offset: 0x268 ( /W)D-Cache Clean by MVA to PoC */
    volatile uint32_t DCCSW;                /*!< Offset: 0x26C ( /W)D-Cache Clean by Set-way */
    volatile uint32_t DCCIMVAC;             /*!< Offset: 0x270 ( /W)D-Cache Clean and Invalidate by MVA to PoC */
    volatile uint32_t DCCISW;               /*!< Offset: 0x274 ( /W)D-Cache Clean and Invalidate by Set-way */
    volatile uint32_t BPIALL;               /*!< Offset: 0x278 ( /W)Branch Predictor Invalidate All */
} bad_scb_typedef_t;

typedef enum{
    BAD_SCB_PRIO_GROUP0 = 0,
    BAD_SCB_PRIO_GROUP1,
    BAD_SCB_PRIO_GROUP2,
    BAD_SCB_PRIO_GROUP3,
    BAD_SCB_PRIO_GROUP4
}bad_scb_prio_grouping_t;

typedef enum{
    BAD_SCB_MEMORY_MANAGEMENT_INTR = 0,
    BAD_SCB_BUS_FAULT_INTR=1,
    BAD_SCB_USAGE_FAULT_INTR=2,
    BAD_SCB_SVC_INTR = 7,
    BAD_SCB_DEBUG_MONITOR_INTR = 8,
    BAD_SCB_PENDSV_INTR = 10,
    BAD_SCB_SYSTICK_INTR = 11
}bad_scb_core_interrupt_t;

typedef enum {
    BAD_SCB_PRIO0 = 0,
    BAD_SCB_PRIO1,
    BAD_SCB_PRIO2,
    BAD_SCB_PRIO3,
    BAD_SCB_PRIO4,
    BAD_SCB_PRIO5,
    BAD_SCB_PRIO6,
    BAD_SCB_PRIO7,
    BAD_SCB_PRIO8,
    BAD_SCB_PRIO9,
    BAD_SCB_PRIO10,
    BAD_SCB_PRIO11,
    BAD_SCB_PRIO12,
    BAD_SCB_PRIO13,
    BAD_SCB_PRIO14,
    BAD_SCB_PRIO15
}bad_scb_interrupt_priority_t;

typedef enum{
    BAD_SCB_FPU_NO_ACCESS = 0,
    BAD_SCB_FPU_PRIV_ACCESS = 5,
    BAD_SCB_FPU_FULL_ACCESS = 15,
}bad_scb_fpu_permission_t;

#define BAD_SCB_BASE (0xE000ED00UL)
#define BAD_SCB ((bad_scb_typedef_t *) BAD_SCB_BASE)

#define BAD_SCB_AIRCR_VECTKEY_SHIFT             16U                                            
#define BAD_SCB_AIRCR_VECTKEY_MASK              (0xFFFF << 16)            
#define BAD_SCB_ICSR_PENDSVSET                  (0x1 << 28 ) 
#define BAD_SCB_AIRCR_PRIGROUP_SHIFT            8                                            
#define BAD_SCB_AIRCR_PRIGROUP_MASK             (7 << BAD_SCB_AIRCR_PRIGROUP_SHIFT)                
#define BAD_SCB_CPACR_FPU_SHIFT                 20U
#define BAD_SCB_CPACR_FPU_MASK                  (0xF << BAD_SCB_CPACR_FPU_SHIFT)
/**
 * \b __fpu_setup
 *
 * Internal function that triggers PendSV exception
 * 
 *
 * This function should not be called by the application
 */
BAD_RTOS_STATIC void __scb_trigger_pendsv(){
    BAD_SCB->ICSR = BAD_SCB_ICSR_PENDSVSET;
}
/**
 * \b __scb_set_priority_grouping
 *
 * Internal function that sets interrupt priority grouping 
 *
 *
 * This function should not be called by the application
 *
 * @param[in] bad_scb_prio_grouping_t prio group
 */
BAD_RTOS_STATIC void __scb_set_priority_grouping(bad_scb_prio_grouping_t prio){
    uint32_t reg_value  =  BAD_SCB->AIRCR;                                                
    reg_value &= ~(BAD_SCB_AIRCR_VECTKEY_MASK | BAD_SCB_AIRCR_PRIGROUP_MASK);  
    reg_value  =  (reg_value | (0x5FA << BAD_SCB_AIRCR_VECTKEY_SHIFT) | (prio << BAD_SCB_AIRCR_PRIGROUP_SHIFT));              
    BAD_SCB->AIRCR =  reg_value;
    __dsb();
    BAD_OPT_BARRIER;
}
/**
 * \b __scb_set_core_interrupt_priority
 *
 * Internal function that sets priority of a specified core interrupt
 * 
 *
 * This function should not be called by the application
 *
 * @param[in] bad_scb_core_interrupt_t interrupt 
 * @param[in] bad_scb_interrupt_priority_t priority
 */
BAD_RTOS_STATIC void __scb_set_core_interrupt_priority(bad_scb_core_interrupt_t intr, bad_scb_interrupt_priority_t prio){
    BAD_SCB->SHP[intr] = prio << 4;
}
/**
 * \b __scb_set_fpu_permission_level
 *
 * Internal function that applies fpu permission level  
 *
 *
 * This function should not be called by the application
 *
 * @param[in] bad_scb_fpu_permission_t permission level 
 */
BAD_RTOS_STATIC void __scb_set_fpu_permission_level(bad_scb_fpu_permission_t perms){
    BAD_SCB->CPACR &= ~(BAD_SCB_CPACR_FPU_MASK);
    BAD_SCB->CPACR |= perms << BAD_SCB_CPACR_FPU_SHIFT;
    __dsb();
    __isb();
} 

#ifdef BAD_RTOS_USE_FPU

//Taken from core_cm33.h from CMSIS, Apache License, Version 2.0 
typedef struct{
    volatile uint32_t FPCCR;
    volatile uint32_t FPCAR;
    volatile uint32_t FPDCR;
    volatile uint32_t MVFR0;
    volatile uint32_t MVFR1;
}bad_fpu_typedef_t;

#define BAD_FPU_BASE (0xE000EF34UL)
#define BAD_FPU ((volatile bad_fpu_typedef_t *)BAD_FPU_BASE)

typedef enum {
    BAD_FPU_FEATURE_DISALOW_UNPRIV_CHANGE = 0,
    BAD_FPU_FEATURE_ALLOW_UNPRIV_CHANGE = 1,
    BAD_FPU_FEATURE_DISABLE_AUTO_STACKING = 0,
    BAD_FPU_FEATURE_ENABLE_AUTO_STACKING = 0x80000000,
    BAD_FPU_FEATURE_DISABLE_LAZY_STACKING = 0,
    BAD_FPU_FEATURE_ENABLE_LAZY_STACKING = 0x40000000
}bad_fpu_features_t;

/**
 * \b __fpu_setup
 *
 * Internal function that applies fpu features
 * 
 *
 * This function should not be called by the application
 *
 * @param[in] bad_fpu_features_t mask of features to enable (enum values orred together)
 */
BAD_RTOS_STATIC void __fpu_setup(bad_fpu_features_t features){
    BAD_FPU->FPCCR = features;
    __dsb();
    __isb();
}
#endif

#ifdef BAD_RTOS_USE_MPU

//Taken from core_cm33.h from CMSIS, Apache License, Version 2.0 
typedef struct
{
    volatile uint32_t TYPE;                   /*!< Offset: 0x000 (R/ )  MPU Type Register */
    volatile uint32_t CTRL;                   /*!< Offset: 0x004 (R/W)  MPU Control Register */
    volatile uint32_t RNR;                    /*!< Offset: 0x008 (R/W)  MPU Region Number Register */
    volatile uint32_t RBAR;                   /*!< Offset: 0x00C (R/W)  MPU Region Base Address Register */
    volatile uint32_t RLAR;                   /*!< Offset: 0x010 (R/W)  MPU Region Limit Address Register */
    volatile uint32_t RBAR_A1;                /*!< Offset: 0x014 (R/W)  MPU Region Base Address Register Alias 1 */
    volatile uint32_t RLAR_A1;                /*!< Offset: 0x018 (R/W)  MPU Region Limit Address Register Alias 1 */
    volatile uint32_t RBAR_A2;                /*!< Offset: 0x01C (R/W)  MPU Region Base Address Register Alias 2 */
    volatile uint32_t RLAR_A2;                /*!< Offset: 0x020 (R/W)  MPU Region Limit Address Register Alias 2 */
    volatile uint32_t RBAR_A3;                /*!< Offset: 0x024 (R/W)  MPU Region Base Address Register Alias 3 */
    volatile uint32_t RLAR_A3;                /*!< Offset: 0x028 (R/W)  MPU Region Limit Address Register Alias 3 */
    uint32_t RESERVED0[1];
    volatile uint32_t MAIR[2];
} bad_mpu_typedef_t;

#define BAD_MPU_BASE (0xE000ED90UL)
#define BAD_MPU ((bad_mpu_typedef_t *)BAD_MPU_BASE)

#define BAD_MPU_CTRL_ENABLE (0x1)
#define BAD_MPU_CTRL_DEFAULT_MAP (0x4)
#define BAD_MPU_MAIR_READ_ALLOCATE  (0x1)
#define BAD_MPU_MAIR_WRITE_ALLOCATE (0x2)
#define BAD_MPU_MAIR_SET_REGION(settings,idx) ((settings)<<((idx) * 8))
#define BAD_MPU_MAIR_NORMAL_SETTING(outer,inner) (((outer) << 4)|(inner))

typedef enum{
    BAD_MPU_MAIR_DEVICE_NGNRNE = 0x0,
    BAD_MPU_MAIR_DEVICE_NGNRE = 0x4,
    BAD_MPU_MAIR_DEVICE_NGRE = 0x8,
    BAD_MPU_MAIR_DEVICE_GRE = 0xC
}bad_mpu_mair_device_states_t;

typedef enum{
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

}bad_mpu_mair_normal_states_t;
/**
 * \b __mpu_enable_with_default_map
 *
 * Internal starts the mpu with default priviledged map enabled
 * 
 *
 * This function should not be called by the application
 */
BAD_RTOS_STATIC void __mpu_enable_with_default_map(){
    __dmb();
    BAD_MPU->CTRL = BAD_MPU_CTRL_ENABLE | BAD_MPU_CTRL_DEFAULT_MAP;
    __dsb();
    __isb();
}

#define BAD_RTOS_MAIR_SETTINGS \
    BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
        BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RA_WA,\
        BAD_MPU_MAIR_NORMAL_WB_NON_TRANSIENT_RA_WA),BAD_RTOS_NORMAL_MAIR_IDX)|\
    BAD_MPU_MAIR_SET_REGION(BAD_MPU_MAIR_NORMAL_SETTING(\
        BAD_MPU_MAIR_NORMAL_NON_CACHEABLE,\
        BAD_MPU_MAIR_NORMAL_NON_CACHEABLE),BAD_RTOS_DMA_BUFF_MAIR_IDX)|\
    BAD_MPU_MAIR_SET_REGION(\
        BAD_MPU_MAIR_DEVICE_NGNRE,BAD_RTOS_DEVICE_MAIR_IDX)

START_TASK_MPU_REGIONS_DEFINITIONS(zeroed) //Fallback zeroed regions array
END_TASK_MPU_REGIONS(zeroed)

//Linker script symbols
extern uint8_t __kernel_bss;
extern uint8_t __ekernel_bss;
extern uint8_t __heap;
extern uint8_t __dma_buffs;

/**
 * \b __bad_rtos_mpu_default_setup
 *
 * Internal function that applies general mpu rules
 * 
 *
 * This function should not be called by the application
 */
BAD_RTOS_STATIC void __bad_rtos_mpu_default_setup(){
    BAD_MPU->MAIR[0] = BAD_RTOS_MAIR_SETTINGS;

    // 0x0, force a fault through overlapping regions lol
    BAD_MPU->RNR = 4;
    BAD_MPU->RBAR = (0x08000000) | BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO;
    BAD_MPU->RLAR = (0x08000000) | BAD_MPU_RLAR_EN| BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_MAIR_IDX);  

    //global data
    BAD_MPU->RNR = 5;
    BAD_MPU->RBAR = (uint32_t)(&__heap) | BAD_MPU_RBAR_AP_PRIV_RW_UNPRIV_RW;
    BAD_MPU->RLAR = ((uint32_t)(&__dma_buffs) - 32) | BAD_MPU_RLAR_EN | BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_MAIR_IDX);

    //flash region
    BAD_MPU->RNR = 6;
    BAD_MPU->RBAR = (0x08000000) | BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_RO;
    BAD_MPU->RLAR = (0x08000000 + BAD_RTOS_FLASH_SIZE) | BAD_MPU_RLAR_EN| BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_MAIR_IDX);
    //kernel data 
    BAD_MPU->RNR = 7;
    BAD_MPU->RBAR = (uint32_t)(&__kernel_bss) | BAD_MPU_RBAR_AP_PRIV_RO_UNPRIV_FAULT;
    BAD_MPU->RLAR = ((uint32_t)(&__ekernel_bss) - 32) | BAD_MPU_RLAR_EN | BAD_MPU_RLAR_SET_MAIR_IDX(BAD_RTOS_NORMAL_MAIR_IDX); 

    __mpu_enable_with_default_map();
}
#endif

#ifdef BAD_RTOS_USE_MSGQ
#if defined (BAD_RTOS_USE_UHEAP) || defined (BAD_RTOS_USE_KHEAP)

bad_rtos_status_t msgq_init(bad_msgq_t *q ,uint32_t size, void* (* alloc_func)(uint32_t size)){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!q||!size|| size & (size-1) || !alloc_func){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
     
    q->msgs = alloc_func( size * sizeof(bad_msg_block_t));
    if(q->msgs == BAD_RTOS_STATUS_ALLOC_FAIL){ 
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_ALLOC_FAIL;
    }

    BAD_OPT_BARRIER;

    q->head = q->tail = 0;
    q->capacity = size;
    
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t msgq_deinit(bad_msgq_t *q,void(* free_func)(void* block,uint32_t size)){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!q || !free_func || !q->capacity){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    uint32_t cap = q->capacity;
    q->capacity = 0;
    
    BAD_OPT_BARRIER;
    
    free_func(q->msgs,cap);
    q->head = 0;
    q->tail = 0;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}
#endif

bad_queue_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback ){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!q || !writeback){
        return BAD_QUEUE_BAD_PARAMETERS;
    }
    if(!q->capacity){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_QUEUE_NOT_INITIALISED;
    }

    if(q->tail==q->head){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_QUEUE_EMPTY;
    }

    BAD_OPT_BARRIER;

    *writeback = *(q->msgs+q->tail);
    
    BAD_OPT_BARRIER;
    
    q->tail = (q->tail+1) & (q->capacity-1);

    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_QUEUE_OK;
}


bad_queue_status_t msgq_post_msg(bad_msgq_t *q, uint32_t signal, void *args){
    uint32_t head,next_head;
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    
    if(!q){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_QUEUE_BAD_PARAMETERS;
    }

    if(!q->capacity){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_QUEUE_NOT_INITIALISED;
    }
    do{
        
        head = __ldrex(&q->head);
        next_head = (head+1) & (q->capacity-1);
        if(q->tail == next_head){
            __clrex();
            CONTEX_SWITCH_BARIER_RELEASE;
            return BAD_QUEUE_OVERRUN;
        }
    }while(__strex(next_head, &q->head)); 

    bad_msg_block_t* block = q->msgs+head;
    
    block->signal = signal;
    block->args = args;
    
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_QUEUE_OK;
}

#endif

#if defined (BAD_RTOS_USE_UHEAP) || defined (BAD_RTOS_USE_KHEAP)
/**
 * \b __buddy_init
 *
 * Internal function that initialises specific buddy allocator control block
 *
 * This function should not be called by the application
 *
 * @param[in] bad_buddy_t* buddy allocator control block
 * @param[in] uint8_t * pointer to allocatable memory
 * @param[in] bad_link_node_t** pointer to allocated free list
 * @param[in] uint32_t minimal order of allocation
 * @param[in] uint32_t maximum order of allocation (heap size)
 * @param[in] uint32_t* pointer to bitmask array
 */
void  __buddy_init(bad_buddy_t *cb,
        uint8_t *heap, 
        bad_link_node_t *freelist,
        uint32_t min_order, 
        uint32_t max_order, 
        uint32_t *bmask)
{
    cb->min_order = min_order;
    cb->max_order = max_order;
    cb->heap = heap;
    cb->free_list = freelist;
    cb->bmask = bmask;
    bad_link_node_t* embedded_node = (bad_link_node_t*)cb->heap;
    cb->free_list->next = embedded_node;
    cb->free_list->prev = embedded_node;
    embedded_node->prev = cb->free_list;
    embedded_node->next = cb->free_list;
    for (uint32_t i = 1; i < max_order - min_order + 1; i++){
        cb->free_list[i].next = &cb->free_list[i];
        cb->free_list[i].prev = &cb->free_list[i];
    }
    cb->heads_bmask = 1;
}
/**
 * \b __buddy_alloc
 *
 * Internal function that allocates a block of memory of a specifed order from a specifed buddy control block
 *
 * This function should not be called by the application
 *
 * @param[in] bad_buddy_t* buddy allocator control block
 * @param[in] uint32_t order of allocation
 *
 * @retval void* allocated block
 */
static void* __buddy_alloc(bad_buddy_t *cb,uint32_t order){
    if(order > cb->max_order ){
        return 0;
    }
    uint32_t idx = cb->max_order  - order;
    uint32_t order_mask = (1 << (idx + 1)) - 1;

    uint32_t picked_idx = 31 - __builtin_clz(cb->heads_bmask & order_mask);
    if(picked_idx == UINT32_MAX){
        return 0;
    }
    
    uint32_t splits = idx - picked_idx;
    uint8_t *block_for_split = (uint8_t*)cb->free_list[picked_idx].next;
    cb->free_list[picked_idx].next = cb->free_list[picked_idx].next->next;
    cb->free_list[picked_idx].next->prev = &cb->free_list[picked_idx];
    cb->heads_bmask ^= (uint32_t)(&cb->free_list[idx] == cb->free_list[idx].next) << picked_idx;
    uint32_t splited_block_size = 1 << (cb->max_order - picked_idx-1);
    bad_link_node_t *unused_block;
    uint32_t bmaskidx, bmask_word, bmask_bit,offset_from_base;
        
    if(picked_idx){
        offset_from_base =  block_for_split - cb->heap;
        bmaskidx = ((1<<(picked_idx-1))-1) + ((offset_from_base) >> ( cb->max_order - picked_idx+1));
        bmask_word = bmaskidx >> 5;
        bmask_bit = bmaskidx & 31;
        cb->bmask[bmask_word] ^= 1 << bmask_bit; 
    }
 
    
    for(uint32_t i = 0; i < splits;i++){
        unused_block = (bad_link_node_t *)(block_for_split+splited_block_size);
        unused_block->next = cb->free_list[picked_idx+1].next;
        cb->free_list[picked_idx+1].next = unused_block;
        unused_block->prev = &cb->free_list[picked_idx+1];
        unused_block->next->prev = unused_block;

        offset_from_base = (uint8_t*)unused_block - cb->heap;
        bmaskidx = ((1<<(picked_idx))-1) + ((offset_from_base) >> ( cb->max_order - picked_idx));
        bmask_word = bmaskidx >> 5;
        bmask_bit = bmaskidx & 31;
        cb->bmask[bmask_word] ^= 1 << bmask_bit;        
        picked_idx++;
        cb->heads_bmask |= (1 << picked_idx);
        splited_block_size>>=1;
    }

    return block_for_split;
}
/**
 * \b __buddy_free
 *
 * Internal function that frees a block of memory of a specifed order from a specifed buddy control block
 *
 * This function should not be called by the application
 *
 * @param[in] bad_buddy_t* buddy allocator control block
 * @param[in] void * block to free
 * @param[in] uint32_t order of allocation
 *
 */
static void __buddy_free(bad_buddy_t *cb,void *block,uint32_t order ){
    
    if(order > cb->max_order){
        return;
    }

    uint32_t curr_order = order; 
    void *curr_block = block;
    uint32_t idx = 0;
    
    while((idx = cb->max_order - curr_order)){
        uint32_t buddy_bitmask = 1ULL << curr_order;
        
        uint32_t offset_from_base = (uint8_t *)curr_block - cb->heap;
        /*                     level                         pair                       */ 
        uint32_t bmaskidx =  ((1<<(idx-1))-1) + ((offset_from_base) >> (curr_order + 1));
        uint32_t bmask_word = bmaskidx >> 5;
        uint32_t bmask_bit = bmaskidx & 31;

          
        cb->bmask[bmask_word] ^= 1 << bmask_bit;
        
        if(cb->bmask[bmask_word] & 1 << bmask_bit){
            break;
        }

        uint32_t buddy_offset = offset_from_base ^ buddy_bitmask;
        uint32_t parent_offset = offset_from_base &(~buddy_bitmask);
        void *buddy_addr = (void*)(cb->heap + buddy_offset);
        void *parent_addr = (void*)(cb->heap + parent_offset); 


        bad_link_node_t *buddy = (bad_link_node_t *)buddy_addr;
        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;
        buddy->prev = 0;
        buddy->next = 0;
        cb->heads_bmask ^= (uint32_t)(&cb->free_list[idx] == cb->free_list[idx].next) << idx;
        curr_order++;
        curr_block = parent_addr;
    }

    bad_link_node_t *final_block = (bad_link_node_t*)curr_block;
    final_block->next = cb->free_list[idx].next;
    final_block->next->prev = final_block;
    cb->free_list[idx].next = final_block;
    final_block->prev = &cb->free_list[idx];
    cb->heads_bmask |= 1 << idx;
}

#endif

#if defined (BAD_RTOS_USE_KHEAP)
/**
 * \b __kernel_free
 *
 * Internal function that allocates a block of memory of a specifed order from kernel_heap
 * Converts the size of the allocation to order of allocation and frees it using __buddy_buddy alloc
 * This function should not be called by the application
 *
 * @param[in] uint32_t size of allocation
 *
 */

BAD_RTOS_STATIC void* __kernel_alloc(uint32_t size){
    uint32_t closest_order;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    if(closest_order < kernel_buddy.min_order){
        closest_order = kernel_buddy.min_order;
    }
    return __buddy_alloc(&kernel_buddy,closest_order );
}
/**
 * \b __kernel_free
 *
 * Internal function that frees a block of memory of a specifed order from kernel_heap
 * Converts the size of the allocation to order of allocation and frees it using __buddy_free
 * This function should not be called by the application
 *
 * @param[in] void * block to free
 * @param[in] uint32_t size of allocation
 *
 */
BAD_RTOS_STATIC void __kernel_free(void *block,uint32_t size){
    uint32_t closest_order;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    if(closest_order < kernel_buddy.min_order){
        closest_order = kernel_buddy.min_order;
    }
    __buddy_free(&kernel_buddy,block,closest_order );

}
#endif
#if defined (BAD_RTOS_USE_UHEAP)
void* user_alloc(uint32_t size){
    CONTEX_SWITCH_BARIER_STORE;
    uint32_t control = __get_control();
    
    if(!(control & 0x1)){
        return (void*)BAD_RTOS_STATUS_ALLOC_FAIL;
    }

    uint32_t closest_order;
    void* block;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    if(closest_order < user_buddy.min_order){
        closest_order = user_buddy.min_order;
    }

    CONTEX_SWITCH_BARIER;
    block = __buddy_alloc(&user_buddy,closest_order );
    CONTEX_SWITCH_BARIER_RELEASE;
    return  block; 

}

void user_free(void *block, uint32_t size){
    CONTEX_SWITCH_BARIER_STORE;
    uint32_t closest_order;
    uint32_t control = __get_control();
    
    if(!(control & 0x1)){
        return;
    }
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    if(closest_order < user_buddy.min_order){
        closest_order = user_buddy.min_order;
    }
    CONTEX_SWITCH_BARIER;
    __buddy_free(&user_buddy,block,closest_order );
    CONTEX_SWITCH_BARIER_RELEASE;

}

#endif
/**
 * \b __tcb_queue_slab_init
 *
 * Internal function that initialises global tcb bitmask slab
 *
 * This function should not be called by the application
 *
 */
BAD_RTOS_STATIC void __tcb_queue_slab_init(){
    tcbslab.free_bitmask = (1UL<<(BAD_RTOS_MAX_TASKS))-1;
}
/**
 * \b __tcb_slab_alloc
 *
 * Internal function that allocates a node from global tcb bitmask slab
 *
 * This function should not be called by the application
 * @retval bad_tcb_t * ptr to allocated tcb node
 */
BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_alloc(){
    if(tcbslab.free_bitmask == 0){
        return (bad_tcb_t*)BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    uint8_t block_idx = __builtin_ctz(tcbslab.free_bitmask);
    tcbslab.free_bitmask &= ~(1UL<<block_idx);
    return tcbslab.node_arr + block_idx;
}
/**
 * \b  __tcb_slab_get_idx_from_ptr
 *
 * Internal function that translates a tcb ptr into its index in tcb slab array 
 *
 * This function should not be called by the application
 * @retval uint8_t index into tcb array if invalid returns 0xFF
 */
BAD_RTOS_STATIC uint8_t __tcb_slab_get_idx_from_ptr(bad_tcb_t *block){
    if(tcbslab.node_arr > block || tcbslab.node_arr + BAD_RTOS_MAX_TASKS < block){
        return 0xFF;
    }
    return block - tcbslab.node_arr;
}
/**
 * \b  __tcb_slab_get_idx_from_ptr
 *
 * Internal function that translates tcb index into a ptr  
 *
 * This function should not be called by the application
 * @retval bad_tcb_t * ptr to tcb if invalid returns 0
 */
BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_get_ptr_from_idx(uint8_t idx){
    if(idx >= BAD_RTOS_MAX_TASKS){
        return 0;
    }
    return tcbslab.node_arr+idx;
}

/**
 * \b __tcb_slab_free
 *
 * Internal function that frees a node from global tcb bitmask slab
 *
 * This function should not be called by the application
 */
BAD_RTOS_STATIC void __tcb_slab_free(bad_tcb_t *tcb){
    uint8_t block_idx = __tcb_slab_get_idx_from_ptr(tcb); 
    if(block_idx >= BAD_RTOS_MAX_TASKS){
        return;
    }
    tcbslab.free_bitmask |= (1ULL<<block_idx); 
}

/**
 * \b __prio_list_enqueue
 *
 * Internal function that enqueues a specifed tcb into a specifed list by its priority
 *
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_link_node_t* queue ptr
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] bad_rtos_misc_t enum value of the specifed queue 
 */
BAD_RTOS_STATIC void __prio_list_enqueue(bad_link_node_t *q,bad_tcb_t *tcb, bad_rtos_misc_t target){

    bad_link_node_t *traverse = q->next;
    bad_link_node_t *prev = q;
    bad_tcb_t *tcb_to_compare = BAD_CONTAINER_OF(traverse,bad_tcb_t,qnode);
    while (traverse && tcb_to_compare->raised_priority <= tcb->raised_priority) {
        prev = traverse;
        traverse = traverse->next;
        tcb_to_compare = BAD_CONTAINER_OF(traverse,bad_tcb_t,qnode);
    }
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode; 
    tcb_qnode_ptr->next = traverse;
    tcb_qnode_ptr->prev = prev;
    tcb->misc = target;
    if(traverse){
        traverse->prev = tcb_qnode_ptr;
    }
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr;
}

/**
 * \b __prio_list_enqueue
 *
 * Internal function that enqueues a specifed tcb into readyq
 *
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_tcb_t* tcb to enqueue
 */
BAD_RTOS_STATIC void __readyq_enqueue(bad_tcb_t *tcb){
    bad_link_node_t *head =  &kernel_cb.readyq[tcb->raised_priority];
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode;
    tcb_qnode_ptr->prev = head->prev;
    tcb_qnode_ptr->next = head;
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr;
    head->prev = tcb_qnode_ptr;
    kernel_cb.ready_bmask |= 1 << tcb->raised_priority;
    tcb->misc = BAD_RTOS_MISC_READYQ_MEMBER;
}
/**
 * \b __get_top_ready_prio
 *
 * Internal function that returns the top priority ready
 *
 * This function should not be called by the application
 */
BAD_RTOS_STATIC uint32_t __get_top_ready_prio(){
    return __builtin_ctz(kernel_cb.ready_bmask);
}
/**
 * \b __readyq_dequeue_head
 *
 * Internal function that dequeues the top priority task from readyq 
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 *
 * @retval bad_tcb_t* pointer to the dequeued head
 */
BAD_RTOS_STATIC bad_tcb_t *__readyq_dequeue_head(){
    uint32_t top = __get_top_ready_prio();
    bad_link_node_t *tcb_qnode_ptr = kernel_cb.readyq[top].next;
    bad_tcb_t *tcb = BAD_CONTAINER_OF(tcb_qnode_ptr, bad_tcb_t, qnode);
    tcb_qnode_ptr->next->prev = tcb_qnode_ptr->prev;
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr->next;
    tcb_qnode_ptr->next = 0;
    tcb_qnode_ptr->prev = 0;
    kernel_cb.ready_bmask ^= (kernel_cb.readyq[top].next ==  &kernel_cb.readyq[top]) << top;
    return tcb;
}

/**
 * \b __delayq_enqueue
 *
 * Internal function that enqueues a specifed tcb into a kernel.delayq delta list
 * 
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] uint32_t ablsolute delay value 
 */

BAD_RTOS_STATIC void __delayq_enqueue(bad_tcb_t *tcb, uint32_t absolute){
    
    bad_link_node_t *traverse = kernel_cb.delayq.next;
    bad_link_node_t *prev = &kernel_cb.delayq;
    bad_tcb_t *traverse_tcb = BAD_CONTAINER_OF(traverse,bad_tcb_t,delaynode);
    uint32_t compound = 0;
    
    while (traverse && (compound+=traverse_tcb->counter) <= absolute) {
        prev = traverse;
        traverse = traverse->next;
        traverse_tcb = BAD_CONTAINER_OF(traverse,bad_tcb_t,delaynode);
    }


    bad_link_node_t *tcb_delaynode_ptr = &tcb->delaynode;

    tcb_delaynode_ptr->next = traverse;
    tcb_delaynode_ptr->prev = prev;
    tcb->delayq_misc = BAD_RTOS_MISC_DELAYQ_MEMBER;
    if(traverse){
        traverse->prev = tcb_delaynode_ptr;
        compound -= traverse_tcb->counter;
    }
    tcb->counter = absolute - compound;
    tcb_delaynode_ptr->prev->next = tcb_delaynode_ptr;
}
/**
 * \b __delayq_dequeue
 *
 * Internal function that dequeues a specifed tcb from a kernel.delayq delta list
 * 
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_tcb_t* tcb to denqueue
 *
 */
BAD_RTOS_STATIC bad_rtos_status_t __delayq_dequeue(bad_tcb_t *tcb){
   
    if(tcb->delayq_misc == BAD_RTOS_MISC_NOT_DELAYED){
        return BAD_RTOS_STATUS_WRONG_Q;
    }
    
    bad_link_node_t *tcb_delaynode_ptr = &tcb->delaynode;
    tcb_delaynode_ptr->prev->next = tcb_delaynode_ptr->next;
    if(tcb_delaynode_ptr->next){
        bad_tcb_t *next_tcb = BAD_CONTAINER_OF(tcb_delaynode_ptr->next,bad_tcb_t,delaynode);
        next_tcb->counter+=tcb->counter;
        tcb_delaynode_ptr->next->prev = tcb_delaynode_ptr->prev;
    }
    tcb_delaynode_ptr->next = 0;
    tcb_delaynode_ptr->prev = 0;
    tcb->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED;
    return BAD_RTOS_STATUS_OK;
}
/**
 * \b __prio_list_dequeue_head
 *
 * Internal function that dequeues the head of the specifed queue
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 * @param[in] bad_link_node_t* queue to dequeue head from
 *
 * @retval bad_tcb_t* pointer to the dequeued head
 * @retval 0 if no nodes available
 */
BAD_RTOS_STATIC bad_tcb_t* __prio_list_dequeue_head(bad_link_node_t *q){
    bad_link_node_t *head = q->next;
    if(!head){
        return 0;
    }
    bad_link_node_t *new_head = head->next; 
    q->next = new_head;
    if(new_head){
        new_head->prev = q;
    }
    return BAD_CONTAINER_OF(head,bad_tcb_t,qnode);
}
/**
 * \b __delayq_dequeue_head
 *
 * Internal function that dequeues the head of the kernel_cb.delayq delta list
 *  
 *
 * This function should not be called by the application
 *
 * @retval bad_tcb_t* pointer to the dequeued head
 */
BAD_RTOS_STATIC bad_tcb_t* __delayq_dequeue_head(){
    bad_link_node_t *head = kernel_cb.delayq.next;
    if(!head){
        return 0;
    }
    bad_link_node_t *new_head = head->next; 
    kernel_cb.delayq.next = new_head;
    if(new_head){
        new_head->prev = &kernel_cb.delayq;
    }
    bad_tcb_t *head_tcb = BAD_CONTAINER_OF(head,bad_tcb_t,delaynode);
    head_tcb->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED; 
    return head_tcb;
}


/**
 * \b __enqueue_head
 *
 * Internal function that enqueues a tcb as a head of the specified queue
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 * @param[in] bad_link_node_t* queue to enqueue head from
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] bad_rtos_misc_t enum value of the specifed queue 
 *
 */
BAD_RTOS_STATIC void __enqueue_head(bad_link_node_t *q, bad_tcb_t *tcb, bad_rtos_misc_t target){
    bad_link_node_t * old_head = q->next;
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode;
    if(old_head){ 
        old_head->prev = tcb_qnode_ptr;
    }
    tcb_qnode_ptr->next = old_head;
    tcb_qnode_ptr->prev = q;
    tcb->misc = target;
    q->next = tcb_qnode_ptr;
}

/**
 * \b __remove_entry
 *
 * Internal function that removes a tcb from a specified queue 
 * If its not in the specified queue (depending on the enum provided) will return BAD_RTOS_STATUS_WRONG_Q
 *   
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 *
 * @param[in] bad_tcb_t* tcb to denqueue
 * @param[in] bad_rtos_misc_t enum value of the specifed queue
 *
 * @retval BAD_RTOS_STATUS_OK successfully removed the tcb from the queue
 * @retval BAD_RTOS_STATUS_WRONG_Q tcb is not in the specified queue
 */
BAD_RTOS_STATIC bad_rtos_status_t __remove_entry(bad_tcb_t *tcb,bad_rtos_misc_t target){
    if(tcb->misc != target){
        return BAD_RTOS_STATUS_WRONG_Q;
    }
    
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode;
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr->next;
    if(tcb_qnode_ptr->next){
        tcb_qnode_ptr->next->prev = tcb_qnode_ptr->prev;
    }

    return BAD_RTOS_STATUS_OK;
}


/**
 * \b __sched_update
 *
 * Internal function that schedules a context switch
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * The tcb running should not be in any queue
 * @param[in] bad_tcb_t* tcb to run
 *
 */
BAD_RTOS_STATIC void __sched_update(bad_tcb_t *tcb){
    kernel_cb.next = tcb;
    tcb->misc = BAD_RTOS_MISC_RUNNING;
    __scb_trigger_pendsv(); 
}

/**
 * \b __sched_try_update
 *
 * Internal function that check if there is a ready task that should preempt the current one or an already scheduled one
 *  
 *
 * This function should not be called by the application
 *
 */
BAD_RTOS_STATIC void __sched_try_update(){
    uint32_t top_ready_prio = __get_top_ready_prio();
    if(kernel_cb.next){
        if(top_ready_prio < kernel_cb.next->raised_priority){
            __readyq_enqueue(kernel_cb.next);
            __sched_update(__readyq_dequeue_head());
        }
        return;
    }

    if(top_ready_prio < kernel_cb.curr->raised_priority){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}
/**
 * \b __sched_try_preempt
 *
 * Internal function that check if a specified task should preempt the current one or an already scheduled one
 * Enqueues the task to readyq otherwise
 * 
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * The tcb running should not be in any queue
 * @param[in] bad_tcb_t* ptr to a candidate task 
 */
BAD_RTOS_STATIC void __sched_try_preempt(bad_tcb_t *tcb){

    if(kernel_cb.next){
        if(tcb->raised_priority < kernel_cb.next->raised_priority){
            __readyq_enqueue(kernel_cb.next);
            __sched_update(tcb);
        }else {
            __readyq_enqueue(tcb);
        }
        return;
    }

    if(tcb->raised_priority < kernel_cb.curr->raised_priority){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(tcb);
    }else {
        __readyq_enqueue(tcb);
    }

}

BAD_RTOS_STATIC void  __task_block(){
    __enqueue_head(&kernel_cb.blockedq, kernel_cb.curr,BAD_RTOS_MISC_BLOCKEDQ_MEMBER);
    __sched_update(__readyq_dequeue_head());
}
/**
 * \b __handle_systick_event
 *
 * Internal function that handles systick event (wakes up tasks pending wakeup , 
 * tries to preempt tasks whose time frame ran out) 
 *
 * This function should not be called by the application
 *
 */

void __handle_systick_event(bad_systick_status_t status){
    if(status > 1){
        do{ 
            bad_tcb_t *wake = __delayq_dequeue_head();

            if(wake->cbptr){
                bad_task_handle_t wake_handle = __tcb_slab_get_idx_from_ptr(wake) | 
                                                    BAD_TASK_HANDLE_GEN(wake->generation);
                wake->cbptr(wake_handle,wake->args);
                wake->cbptr = 0;
                wake->args = 0;
            }

            wake->counter = wake->ticks_to_change;
            __readyq_enqueue(wake);
        }while(kernel_cb.delayq.next && !BAD_CONTAINER_OF(kernel_cb.delayq.next, bad_tcb_t, delaynode)->counter);

    }
    if (status & BAD_SYSTICK_TIMEFRAME_PENDING) {
        kernel_cb.curr->counter = kernel_cb.curr->ticks_to_change;
    }    

    uint32_t top_ready_prio = __get_top_ready_prio(); 
    
    if(kernel_cb.next){
        if(top_ready_prio < kernel_cb.next->raised_priority){
            __readyq_enqueue(kernel_cb.next);
            __sched_update(__readyq_dequeue_head());
        }
        return;
    }

    if(kernel_cb.ready_bmask && 
        top_ready_prio + (status == BAD_SYSTICK_DELAY_WAKE_PENDING)
        <= kernel_cb.curr->raised_priority ){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}
/**
 * \b __init_stack
 *
 * Internal function that initialises the inital stack of the task
 *
 * This function should not be called by the application
 *
 */
BAD_RTOS_STATIC uint32_t * __init_stack(void (*task)(), uint32_t *stacktop,void *args){
    *--stacktop = 0x01000000UL;     // xPSR (Thumb bit set)
    *--stacktop = (uint32_t)task|0x1;   // PC
    *--stacktop = 0x0;     
    *--stacktop = 0x12121212UL;     // R12
    *--stacktop = 0x03030303UL;     // R3
    *--stacktop = 0x02020202UL;     // R2
    *--stacktop = 0x01010101UL;     // R1
    *--stacktop = (uint32_t)args;     // R0 (parameter, optional)
    *--stacktop = 0xFFFFFFFDUL;  //lr pushed to track fpu state (thread mode + PSP + No FPU)
    for(uint8_t i = 0;i < 8;i++){
        *--stacktop = 0xDEADBEEFUL;
    }
    return stacktop;
}


BAD_RTOS_STATIC bad_task_handle_t __task_make(bad_task_descr_t *args){
#ifdef BAD_RTOS_USE_FPU
    if(args->stack_size < 128 || args->stack_size % 32){
        return BAD_TASK_HANDLE_INVALID_HANDLE(BAD_RTOS_STATUS_ALLOC_FAIL); 
    }
#else
    if(args->stack_size < 64 || args->stack_size % 32){
        return BAD_TASK_HANDLE_INVALID_HANDLE(BAD_RTOS_STATUS_ALLOC_FAIL); 
    }
#endif

    bad_tcb_t *new_task = __tcb_slab_alloc();
    uint8_t new_task_idx = __tcb_slab_get_idx_from_ptr(new_task);
    if(!new_task){
        return BAD_TASK_HANDLE_INVALID_HANDLE(BAD_RTOS_STATUS_ALLOC_FAIL); 
    }
    new_task->entry = args->entry;
    if(args->base_priority > IDLE_TASK_PRIO){
        return BAD_TASK_HANDLE_INVALID_HANDLE(BAD_RTOS_STATUS_BAD_PARAMETERS);
    }
    new_task->base_priority = args->base_priority;
    new_task->raised_priority = args->base_priority;
#if defined (BAD_RTOS_USE_KHEAP)
    if(!args->stack){
        new_task->stack = __kernel_alloc(args->stack_size);
        if(!new_task->stack){
            __tcb_slab_free(new_task);
            return BAD_TASK_HANDLE_INVALID_HANDLE(BAD_RTOS_STATUS_ALLOC_FAIL); 
        }
        new_task->dyn_stack = 1;
    }else{
        new_task->dyn_stack = 0;
        new_task->stack = args->stack;
    }

#else
    if(!args->stack){
        return BAD_TASK_HANDLE_INVALID_HANDLE(BAD_RTOS_STATUS_BAD_PARAMETERS);    
    }
    new_task->stack = args->stack;
#endif
#ifdef BAD_RTOS_USE_MPU
    if(!args->regions){
        new_task->regions = zeroed_regions;
    }else{
        new_task->regions = args->regions;
    }
#endif 
    new_task->stack_size = args->stack_size;
    new_task->ticks_to_change = args->ticks_to_change;
    new_task->counter = args->ticks_to_change;
    new_task->sp = __init_stack(new_task->entry, new_task->stack + (args->stack_size/sizeof(uint32_t)),args->args);
    if(kernel_cb.is_running ){
        __sched_try_preempt(new_task);
    }else{
        __readyq_enqueue(new_task);
    }
    bad_task_handle_t handle = new_task_idx | (new_task->generation << 16);
    return handle;
}


BAD_RTOS_STATIC bad_rtos_status_t __task_unblock(bad_task_handle_t handle){
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    if(!tcb || tcb->generation != BAD_TASK_HANDLE_GET_GEN(handle)){
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    if(__remove_entry(tcb, BAD_RTOS_MISC_BLOCKEDQ_MEMBER)!= BAD_RTOS_STATUS_OK){
        return BAD_RTOS_STATUS_NOT_BLOCKED;
    }
    __sched_try_preempt(tcb);
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __task_delay_cancel(bad_task_handle_t handle){
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    if(!tcb || tcb->generation != BAD_TASK_HANDLE_GET_GEN(handle)){
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }

    if(__delayq_dequeue(tcb)!= BAD_RTOS_STATUS_OK){
        return BAD_RTOS_STATUS_NOT_DELAYED;
    }

    tcb->cbptr = 0;
    tcb->args = 0;
    *(tcb->sp+9) = BAD_RTOS_STATUS_WOKEN;
    __sched_try_preempt(tcb);
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __task_yield(){
    uint32_t top_ready_prio = __get_top_ready_prio();
    if(top_ready_prio == kernel_cb.curr->raised_priority){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
        return BAD_RTOS_STATUS_OK;
    }
    else{
        return BAD_RTOS_STATUS_CANT_YEILD;
    }
}

#ifdef BAD_RTOS_USE_MUTEX
bad_rtos_status_t mutex_init(bad_mutex_t *mut){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!mut){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    mut->blockedq = (bad_link_node_t){0};
    mut->owner = 0;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

/**
 * \b __mutex_timeout_cb
 *
 * Internal callback function, run when timeout for accuiring a mutex is reached
 *
 * This function should not be called by the application
 *
 */
BAD_RTOS_STATIC void __mutex_timeout_cb(bad_task_handle_t handle ,void *mutex){
    bad_mutex_t * mut = (bad_mutex_t* ) mutex;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    __remove_entry(tcb,BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

BAD_RTOS_STATIC bad_rtos_status_t __mutex_delete(bad_mutex_t *mut){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(kernel_cb.curr != mut->owner ){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }

    kernel_cb.curr->mutex_count--;
    if(!kernel_cb.curr->mutex_count){
        kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
    }
    mut->owner = 0;
    
    if(!mut->blockedq.next){
        return BAD_RTOS_STATUS_OK;
    }
    
    bad_link_node_t *traverse = mut->blockedq.next;
    bad_tcb_t *traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    while(traverse){
        *(traverse_tcb->sp+9) = BAD_RTOS_STATUS_DELETED;
        if(traverse_tcb->cbptr == __mutex_timeout_cb){
            traverse_tcb->cbptr = 0;
            traverse_tcb->args = 0;
            __delayq_dequeue(traverse_tcb);
        }
        traverse = traverse->next;
        __readyq_enqueue(traverse_tcb);
        traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    }

    mut->blockedq = (bad_link_node_t){0};

    __sched_try_update();

    return BAD_RTOS_STATUS_OK;
}
/**
 * \b __mutex_update_owner_pos
 *
 * Internal function, run to update the position  of the owner when priority is raised
 *
 * This function should not be called by the application
 *
 */
BAD_RTOS_STATIC void __mutex_update_owner_pos(bad_tcb_t *owner){
    if(owner->misc == BAD_RTOS_MISC_READYQ_MEMBER){
        __remove_entry(owner,BAD_RTOS_MISC_READYQ_MEMBER);
        __readyq_enqueue(owner);
    }
}

BAD_RTOS_STATIC bad_rtos_status_t __mutex_take(bad_mutex_t *mut, uint32_t delay){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!mut->owner){
        mut->owner = kernel_cb.curr;
        kernel_cb.curr->mutex_count++;
        return BAD_RTOS_STATUS_OK;
    }
    
    if(mut->owner == kernel_cb.curr){
        return BAD_RTOS_STATUS_RECURSIVE_TAKE;
    }

    if(delay == UINT32_MAX){
        return BAD_RTOS_STATUS_WOULD_BLOCK;
    }

    if(delay){
        kernel_cb.curr->args = mut;
        kernel_cb.curr->cbptr = __mutex_timeout_cb;
        __delayq_enqueue( kernel_cb.curr, delay);
    }

    __prio_list_enqueue(&mut->blockedq,kernel_cb.curr, BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER);
    __sched_update(__readyq_dequeue_head());
    if(kernel_cb.curr->raised_priority < mut->owner->raised_priority){
        mut->owner->raised_priority = kernel_cb.curr->raised_priority;
        __mutex_update_owner_pos(mut->owner);
    }


    return BAD_RTOS_STATUS_OK;
}   


BAD_RTOS_STATIC bad_rtos_status_t __mutex_put(bad_mutex_t *mut){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(kernel_cb.curr!= mut->owner){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }

    mut->owner = __prio_list_dequeue_head(&mut->blockedq);
    if(!--kernel_cb.curr->mutex_count){
        kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
    }

    if(!mut->owner){ 
        return BAD_RTOS_STATUS_OK; 
    }
    mut->owner->mutex_count++;
    
    if(mut->owner->cbptr == __mutex_timeout_cb){
        mut->owner->cbptr = 0;
        mut->owner->args = 0;
        __delayq_dequeue(mut->owner);
    }
         
    *(mut->owner->sp+9) = BAD_RTOS_STATUS_OK;
    
    __sched_try_preempt(mut->owner);

    return BAD_RTOS_STATUS_OK;
}
#endif


#ifdef BAD_RTOS_USE_SEMAPHORE
bad_rtos_status_t sem_init(bad_sem_t *sem,uint16_t reset_value){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!sem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    sem->blockedq = (bad_link_node_t){0};
    sem->counter = reset_value;
    sem->init_flag = 1;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}


/**
 * \b __mutex_timeout_cb
 *
 * Internal callback function, run when timeout for accuiring a semaphore is reached
 *
 * This function should not be called by the application
 *
 */
BAD_RTOS_STATIC void __sem_timeout_cb(bad_task_handle_t handle ,void *semaphore){
    bad_sem_t *sem = (bad_sem_t* ) semaphore;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    __remove_entry(tcb,BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_delete(bad_sem_t *sem){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }

    if(!sem->init_flag){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }

    sem->counter = 0;
    sem->init_flag = 0;

    if(!sem->blockedq.next){
        return BAD_RTOS_STATUS_OK;
    }
    
    bad_link_node_t *traverse = sem->blockedq.next;
    bad_tcb_t *traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    while(traverse){
        *(traverse_tcb->sp+9) = BAD_RTOS_STATUS_DELETED;
        if(traverse_tcb->cbptr == __sem_timeout_cb){
            traverse_tcb->cbptr = 0;
            traverse_tcb->args = 0;
            __delayq_dequeue(traverse_tcb);
        }
        traverse = traverse->next;
        __readyq_enqueue(traverse_tcb);
        traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    }

    sem->blockedq = (bad_link_node_t){0};

    __sched_try_update();

    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_take(bad_sem_t *sem, uint32_t delay){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!sem->init_flag){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    if(!sem->counter){
        if(delay == UINT32_MAX){
            return BAD_RTOS_STATUS_WOULD_BLOCK;
        }

        if(delay){
            kernel_cb.curr->args = sem;
            kernel_cb.curr->cbptr = __sem_timeout_cb;
            __delayq_enqueue( kernel_cb.curr, delay);
        }

        __prio_list_enqueue(&sem->blockedq,kernel_cb.curr, BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER);
        __sched_update(__readyq_dequeue_head());
        return BAD_RTOS_STATUS_OK;
    }
    sem->counter--;

    return BAD_RTOS_STATUS_OK;
}   


BAD_RTOS_STATIC bad_rtos_status_t __sem_put(bad_sem_t *sem){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }

    if(!sem->init_flag){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }

    // wakes a task and returns a success code using stacked registers
    if(sem->blockedq.next){
        bad_tcb_t * tcb = __prio_list_dequeue_head(&sem->blockedq);
        if(tcb->cbptr == __sem_timeout_cb){
            __delayq_dequeue(tcb);
            tcb->cbptr = 0;
            tcb->args = 0;
        }
        *(tcb->sp+9) = BAD_RTOS_STATUS_OK;
        __sched_try_preempt(tcb);
        return BAD_RTOS_STATUS_OK;
    }

    sem->counter++;
    return BAD_RTOS_STATUS_OK;
}
// you dont need to call this if you initialise it yourself on .data 
bad_rtos_status_t nbsem_init(bad_nbsem_t *nbsem, uint32_t reset_value){ 
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    nbsem->counter = reset_value;
    nbsem->init_flag = 1;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t nbsem_take(bad_nbsem_t *nbsem){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
 
    if(!nbsem->init_flag){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    uint32_t counter;
    //isr preemption safe decrement
    do{
        counter = __ldrex(&nbsem->counter);
        if(!counter){
            __clrex();
            CONTEX_SWITCH_BARIER_RELEASE;
            return BAD_RTOS_STATUS_WOULD_BLOCK;
        }
    }while(__strex(counter-1, &nbsem->counter));

    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t nbsem_put(bad_nbsem_t *nbsem){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }

    if(!nbsem->init_flag){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }

    uint32_t counter;
    do{
        counter = __ldrex(&nbsem->counter);
    }while(__strex(counter+1, &nbsem->counter));

    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t nbsem_delete(bad_nbsem_t *nbsem){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }

    //basically just resets the semaphore, leaving memory operations to the program
    nbsem->counter = 0;
    nbsem->init_flag = 0;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

#endif

BAD_RTOS_STATIC void __task_finish(){
#ifdef BAD_RTOS_USE_MUTEX
    if(kernel_cb.curr->mutex_count){ //trap when task want to finish without releasing mutexes
        __builtin_trap();
        return;
    }
#endif 
#if defined (BAD_RTOS_USE_KHEAP)
    if(kernel_cb.curr->dyn_stack){ //free the dynamically allocated stack 
        __kernel_free((void*)kernel_cb.curr->stack,kernel_cb.curr->stack_size);
    }
#endif
    if(kernel_cb.next){
        goto skip_resched;
    }
    __sched_update(__readyq_dequeue_head());
skip_resched:
    kernel_cb.curr->generation++;
    __tcb_slab_free(kernel_cb.curr);//free the the tcb used by task
}

BAD_RTOS_STATIC void __task_delay(uint32_t delay,cbptr cb, void* args){
    kernel_cb.curr->cbptr =cb;
    kernel_cb.curr->args = args;
    __delayq_enqueue(kernel_cb.curr,delay);
    __sched_update(__readyq_dequeue_head());
   
}

// ASM stuff

//idle task
__asm__(
".thumb_func            \n"
".global idle_task      \n"
"idle_task:             \n"
"infinite_loop:         \n"
    "dsb                \n"
    "wfi                \n"
    "b infinite_loop    \n"
);
//svc calls (upper nibble indicates if it can be called from thread context 
//and the lower one indicates if it can be called from isr)
__asm__(
".thumb_func        \n"
".global task_make  \n"
"task_make:         \n"
    "svc 0x11       \n"
    "bx lr          \n"
);

__asm__(
".thumb_func            \n"
".global task_unblock   \n"
"task_unblock:          \n"
    "svc 0x22           \n"
    "bx lr              \n"
);

__asm__(
".thumb_func                \n"
".global task_delay_cancel  \n"
"task_delay_cancel:         \n"
    "svc 0x33               \n"
    "bx lr                  \n"

);

__asm__(
".thumb_func                \n"
".global __first_task_start \n"
"__first_task_start:        \n"
    "svc 0x4                \n" 
    "bx lr                  \n"
);

__asm__(
".thumb_func            \n"
".global task_finish    \n"
"task_finish:           \n"
    "svc 0x40           \n"
    "bx lr              \n"
);

__asm__(
".thumb_func        \n"
".global task_yield \n"
"task_yield:        \n"
    "svc 0xD0       \n"
    "bx lr          \n"
);

__asm__(
".thumb_func        \n"
".global task_block \n"
"task_block:        \n"
    "svc 0xE0       \n"
    "bx lr          \n"
);

__asm__(
".thumb_func        \n"
".global task_delay \n"
"task_delay:        \n"
    "svc 0xF0       \n"
    "bx lr          \n"
);

#ifdef BAD_RTOS_USE_KHEAP
__asm__(
".thumb_func            \n"
".global kernel_alloc   \n"
"kernel_alloc:          \n"
    "svc 0x55           \n"
    "bx lr              \n"
);

__asm__(
".thumb_func            \n"
".global kernel_free    \n"
"kernel_free:           \n"
    "svc 0x66           \n"
    "bx lr              \n"
);
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
__asm__(
".thumb_func        \n"
".global sem_take   \n"
"sem_take:          \n"
    "svc 0x77       \n"
    "bx lr          \n"
);
__asm__(
".thumb_func        \n"
".global sem_put    \n"
"sem_put:           \n"
    "svc 0x88       \n"
    "bx lr          \n"
);

__asm__(
".thumb_func        \n"
".global sem_delete \n"
"sem_delete:        \n"
    "svc 0x99       \n"
    "bx lr          \n"
);
#endif

#ifdef BAD_RTOS_USE_MUTEX
__asm__(
".thumb_func            \n"
".global mutex_delete   \n"
"mutex_delete:          \n"
    "svc 0xA0           \n"
    "bx lr              \n"
);
__asm__(
".thumb_func            \n"
".global mutex_put      \n"
"mutex_put:             \n"
    "svc 0xB0           \n"
    "bx lr              \n"
);

__asm__(
".thumb_func            \n"
".global mutex_take     \n"
"mutex_take:            \n"
    "svc 0xC0           \n"
    "bx lr              \n"
);
#endif

// svc isr ,masks lower nibble if the call was from thread context and the higher one if the call was from isr

void __attribute__((naked)) BAD_RTOS_SVC_HANDLER_NAME(){
    __asm__ volatile(
        "tst     lr, #4         \n"
        "beq     isr            \n"
    "thread:                    \n"
        "mrs     r1, psp        \n"
        "mov     r2, #0xF0      \n"      
        "b       common         \n"
    "isr:                       \n"
        "mrs     r1, msp        \n"
        "mov     r2, #0x0F      \n"    
    "common:                    \n"
        "ldr     r3, [r1,#24]   \n"
        "sub     r3, #2         \n"
        "ldrb    r0, [r3]       \n"
        "and     r0, r2         \n"
#ifdef BAD_RTOS_USE_MPU
        "cmp r0,#0xf            \n"
        "beq already_unlocked   \n"
        "sub sp,#16             \n"
        "stmia sp,{r4,r5,lr}    \n"
        "ldr r4, =%0            \n"
        "ldr r5, [r4]           \n"
        "bic r2,r5,#1           \n"
        "str r2,[r4]            \n"
        "dsb                    \n"
        "isb                    \n"
        "bl       svc_c         \n"
        "str r5,[r4]            \n"
        "ldmia sp,{r4,r5,lr}    \n"
        "add sp,#16             \n"
        "bx lr                  \n"
#endif
"already_unlocked:              \n"
        "b svc_c                \n"
        ".ltorg                 \n"
        :
        :
#ifdef BAD_RTOS_USE_MPU
        "i"(&BAD_MPU->RLAR)
#endif
        :
    );
}

//pendsv isr
void __attribute__((naked)) BAD_RTOS_PENDSV_HANDLER_NAME(){ 
    asm volatile(
        "movs r0,%0             \n"
        "msr basepri,r0         \n"
        "isb                    \n"
        "mrs r0,psp             \n"
#ifdef BAD_RTOS_USE_FPU
        "tst lr,#0x10           \n"
        "it eq                  \n"
        "vstmdbeq r0!, {s16-s31}\n"
#endif
        "stmdb r0!, {r4-r11,lr} \n"
#ifdef BAD_RTOS_USE_MPU
        "ldr r12,=%2            \n"
        "ldr r11,[r12,#8]       \n"
        "bic r10,r11,#1         \n"
        "str r10,[r12,#8]       \n"
        "movs r7,#0             \n"
        "str r7,[r12]           \n"
        "dsb                    \n"
        "isb                    \n"
#endif
        "ldr r4,=%1             \n"
        "ldr r3,[r4,#4]         \n"
        "str r0,[r3]            \n"
        "ldr r1,[r4,#8]         \n" 
        "ldr r0,[r1]            \n"  
        "movs r2,#0             \n"
        "str r2,[r4,#8]         \n"
        "str r1,[r4,#4]         \n"
#ifdef BAD_RTOS_USE_MPU
        "ldr r3, [r1,#4]        \n"
        "ldr r1,[r1,#48]        \n"
        "add r2,r12,#4          \n"
        "msr psplim, r3         \n"
        "ldmia r1!,{r3-r10}     \n"
        "stmia r2!,{r3-r10}     \n"
        "mov r1,#7              \n"
        "str r1,[r12]           \n"
        "str r11,[r12,#8]       \n"
        "dsb                    \n"
        "isb                    \n"
#endif
        "ldmia r0!, {r4-r11,lr} \n"
#ifdef BAD_RTOS_USE_FPU
        "tst lr,#0x10           \n"
        "it eq                  \n"
        "vldmiaeq r0!, {s16-s31}\n"
#endif
        "msr psp,r0             \n"
        "movs r2,#2             \n"
        "msr basepri,r2         \n"
        "bx lr                  \n"
        ".ltorg                 \n"
        :
        :"i" (CRITICAL_MASK), "i"(&kernel_cb)
#ifdef BAD_RTOS_USE_MPU
        ,"i"(&BAD_MPU->RNR)
#endif
        : 
    );
}


void __attribute__((naked)) __init_second_stage(){
    __asm__ volatile(
        "ldr r1,=__estack       \n"
        "msr msp,r1             \n"
        "ldr r1,=kernel_cb      \n"
        "ldr r1,[r1,#4]         \n"
        "ldr r0,[r1]            \n"
#ifdef BAD_RTOS_USE_MPU
        "ldr r12,=%0            \n"
        "mov r3,#0              \n"
        "str r3,[r12]           \n"
        "add r2,r12,#4          \n"
        "ldr r3, [r1,#4]        \n"
        "msr psplim, r3         \n"
        "ldr r1,[r1,#48]        \n"
        "ldmia r1!,{r3-r10}     \n"
        "stmia r2!,{r3-r10}     \n"
        "mov r3,#7              \n"
        "str r3,[r12]           \n"
        "ldr r7,[r12,#8]        \n"
        "orrs r7,#1             \n"
        "str r7,[r12,#8]        \n"
        "dsb                    \n"
        "isb                    \n"
#endif
        "ldmia r0!,{r4-r11,lr}  \n"
        "msr psp, r0            \n"
        "bx lr                  \n"
        ".ltorg                 \n"
        :
        :
#ifdef BAD_RTOS_USE_MPU
        "i"(&BAD_MPU->RNR)
#endif
        :
    );
}
void __attribute__((naked)) BAD_RTOS_TICK_HANDLER_NAME(){
    __asm__ volatile(
        "ldr r3,=%0         \n"
        "ldrb r0,[r3,#20]   \n"
        "cbz r0, exit       \n"
        "b cont             \n"
"exit:                      \n"
        "bx lr              \n"
"cont:                      \n"

        "movs r0,%1         \n"
        "msr basepri,r0     \n"
#ifdef BAD_RTOS_USE_MPU
        "push {r4,r5}       \n"
        "ldr r5, =%2        \n"
        "ldr r4,[r5]        \n"
        "bic r0,r4,#1       \n"
        "str r0,[r5]        \n"
        "dsb                \n"
#endif 
        "isb                \n"
        "movs r0,#0         \n"
        "ldr r1,[r3]        \n"
        "adds r1,#1         \n"
        "str r1,[r3]        \n"
        "ldr r1,[r3,#8]     \n"
        "cbnz r1, skip_curr \n"
        "ldr r2,[r3,#4]     \n"
        "ldr r1,[r2,#44]    \n"
        "subs r1,#1         \n"
        "str r1,[r2,#44]    \n"
        "it eq              \n"
        "orreq r0,#1        \n"
"skip_curr:                 \n"
        "ldr r2,[r3,#16]    \n"
        "cbnz r2,nz_delayq  \n"
        "b skip_delayq      \n"
"nz_delayq:                 \n"
        "ldr r1,[r2,#12]    \n"
        "subs r1,#1         \n"
        "str r1,[r2,#12]    \n"
        "it eq              \n"
        "orreq r0,#2        \n"
"skip_delayq:               \n"
        "cbnz r0,back_branch\n"
"back:                      \n"
#ifdef BAD_RTOS_USE_MPU
        "str r4,[r5]        \n"
        "pop {r4,r5}        \n"
        "dsb                \n"
#endif
        "movs r0,#0         \n"
        "msr basepri,r0     \n"
        "bx lr              \n"
"back_branch:               \n"
        "svc 0xf            \n"
        "b back             \n"
        ".ltorg             \n"
        :
        : "i" (&kernel_cb),"i"(CRITICAL_MASK)
#ifdef BAD_RTOS_USE_MPU
        , "i" (&BAD_MPU->RLAR)
#endif
        :
    );
}


//helpers for specific common operations

static inline __attribute__((always_inline)) uint32_t __ldrex(volatile uint32_t* addr){
    uint32_t res;
    __asm__ volatile ("ldrex %0, %1" : "=r"(res): "Q"(*addr): "memory");
    return res;
}

static inline __attribute__((always_inline)) uint32_t __strex(uint32_t val,volatile uint32_t * addr){
    uint32_t res;
    __asm__ volatile ("strex %0, %2, %1" : "=&r" (res), "=Q" (*addr) : "r" (val));
    return res;
}

static inline __attribute__((always_inline)) void __clrex(){
    __asm__ volatile ("clrex":::"memory");
}

static inline __attribute__((always_inline)) void __dmb(){
    __asm__ volatile ("dmb":::"memory");
}

static inline __attribute__((always_inline)) void __dsb(){
    __asm__ volatile ("dsb":::"memory");
}

static inline __attribute__((always_inline)) void __isb(){
    __asm__ volatile ("isb":::"memory");
}

static inline __attribute__((always_inline)) uint32_t __modify_basepri(uint32_t new_basepri)
{
  uint32_t old_basepri;
  __asm__ volatile (
        "mrs %0, basepri    \n"     
        "msr basepri, %1    \n"  
        "isb                \n"
        : "=&r" (old_basepri)
        : "r"   (new_basepri)
        : "memory"
  );
  return old_basepri;
}

static inline __attribute__((always_inline)) void __restore_basepri(uint32_t new_basepri){
    __asm__ volatile(
        "msr basepri, %0    \n"
        "isb                \n"
        : : "r"(new_basepri)
        : "memory"
    );
}

static inline __attribute__((always_inline)) void __set_control(uint32_t new_control){
    __asm__ volatile(
        "msr control, %0    \n"
        "isb                \n"
        : : "r"(new_control)
        : "memory"
    ); 
}

static inline __attribute__((always_inline)) uint32_t __get_control(){
    uint32_t res;
    __asm__ volatile("mrs %0, control":"=r"(res));
    return res;
}

/**
 * \b svc_c
 *
 * Internal function, part of svc handler.
 * Dispatches svc codes to appropriate handlers
 *
 * This function should not be called by the application
 *
 */

static void __attribute__((used)) svc_c(uint8_t svc, uint32_t* stack){
    switch (svc) {
        //start first task
        case 0x4:{
            kernel_cb.is_running = 1;
            __set_control(0x1);
            __restore_basepri(0);
            kernel_cb.curr = __readyq_dequeue_head();
            __asm volatile("b __init_second_stage");
            break;
        }
        case 0x1:
        case 0x10:{

            stack[0] = (uint32_t)__task_make((bad_task_descr_t*)stack[0]);
            break;
        }
        case 0x2:
        case 0x20:{
           stack[0]=__task_unblock(stack[0]);
           break;
        }
        case 0x3:
        case 0x30:{
            stack[0] =__task_delay_cancel(stack[0]);
            break;
        }
        case 0x40:{
            __task_finish();
            stack[0] = BAD_RTOS_STATUS_CANT_FINISH;
            break;
        }

#if defined (BAD_RTOS_USE_KHEAP)
        case 0x5:
        case 0x50:{
            stack[0]=(uint32_t)__kernel_alloc(stack[0]);
            break;
        }
        case 0x6:
        case 0x60:{
            __kernel_free((void*)stack[0], stack[1]);
            break;
        } 
#endif  
       
#ifdef BAD_RTOS_USE_SEMAPHORE
        case 0x7:{
            stack[0] = __sem_take((bad_sem_t*)stack[0], -1);
            break;
        }
        case 0x70:{
            stack[0] = __sem_take((bad_sem_t*)stack[0] , stack[1]);
            break;
        }
        case 0x8:
        case 0x80:{
            stack[0] = __sem_put((bad_sem_t *)stack[0]);
            break;
        }
        case 0x9: 
        case 0x90:{
           stack[0] = __sem_delete((bad_sem_t*)stack[0]);
           break;
        }
#endif
#ifdef BAD_RTOS_USE_MUTEX
        case 0xA0:{
            stack[0] = __mutex_delete((bad_mutex_t*)stack[0]);
            break;
        }
        case 0xB0:{
            stack[0] = __mutex_put((bad_mutex_t*)stack[0]);
            break;
        }
        case 0xC0:{
            stack[0] = __mutex_take((bad_mutex_t*)stack[0], stack[1]);
            break;
        }
#endif
        case 0xD0:{
            stack[0] = __task_yield();
            break;
        }
        case 0xE0:{
            __task_block();            
            stack[0] = BAD_RTOS_STATUS_OK;              
            break;
        }
        case 0xF0:{
            __task_delay(stack[0], (cbptr) stack[1] ,(void*)stack[2]);
            stack[0] = BAD_RTOS_STATUS_OK;
            break;
        }
        case 0xF:{
            __handle_systick_event(stack[0]);
            break;
        }
        case 0:{
            stack[0] = BAD_RTOS_STATUS_WRONG_CONTEXT;
            break;
        }
        default:{
            __builtin_unreachable();
        }
    }
}

BAD_RTOS_STATIC void __bad_rtos_interrupt_setup(){
    __scb_set_priority_grouping(BAD_SCB_PRIO_GROUP4);
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_RTOS_SVC_PRIO);
    __scb_set_core_interrupt_priority(BAD_SCB_PENDSV_INTR, BAD_SCB_PRIO15);
}

// BAD_RTOS_STATIC void __bad_rtos_systick_setup(){
//     systick_setup(CLOCK_SPEED/1000, SYSTICK_FEATURE_CLOCK_SOURCE|SYSTICK_FEATURE_TICK_INTERRUPT);
//     systick_enable();
// }

BAD_RTOS_STATIC void __bad_rtos_readyq_setup(){
    for (uint32_t i = 0; i < BAD_RTOS_PRIO_COUNT; i++){
        kernel_cb.readyq[i].next = &kernel_cb.readyq[i];
        kernel_cb.readyq[i].prev = &kernel_cb.readyq[i];
    }
}

// declaration for user setup function
void bad_user_setup();
/**
 * \b bad_rtos_start
 *
 * Function to start the rtos operation, run this after all the peripheral setup 
 *
 * This function will never return
 *
 */
#define BAD_RTOS_FPU_SETTINGS (BAD_FPU_FEATURE_ENABLE_LAZY_STACKING|BAD_FPU_FEATURE_ENABLE_AUTO_STACKING)
void bad_rtos_start(){
    __restore_basepri(CRITICAL_MASK);
    __tcb_queue_slab_init();
    __bad_rtos_readyq_setup();
#if defined (BAD_RTOS_USE_KHEAP)
    __buddy_init(&kernel_buddy, kheap, kfreelist, KMIN_ORDER, KMAX_ORDER, kbitmask);
#endif
#if defined (BAD_RTOS_USE_UHEAP) 
    __buddy_init(&user_buddy, uheap, ufreelist, UMIN_ORDER, UMAX_ORDER, ubitmask);
#endif 
#ifdef BAD_RTOS_USE_MPU
    __bad_rtos_mpu_default_setup();
#endif
#if defined(BAD_RTOS_USE_FPU) 
    __scb_set_fpu_permission_level(BAD_SCB_FPU_FULL_ACCESS);
#endif
#if defined(BAD_RTOS_USE_FPU) && defined(BAD_RTOS_FPU_DEFAULT_SETTINGS)
    __fpu_setup(BAD_RTOS_FPU_SETTINGS);
#endif
    __bad_rtos_interrupt_setup();
    //__bad_rtos_systick_setup();
    bad_task_descr_t idle_task_descr = {
        .stack = (uint32_t *)idle_stack,
        .stack_size = IDLE_TASK_STACK_SIZE,
        .entry = idle_task,
        .ticks_to_change = UINT32_MAX,
        .base_priority = IDLE_TASK_PRIO
    };
    task_make(&idle_task_descr);
    bad_user_setup();
    __first_task_start();
    
}
#endif

#endif
