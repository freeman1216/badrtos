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
 */
#pragma once
#ifndef BAD_RTOS_CORE
#define BAD_RTOS_CORE


#ifdef BAD_RTOS_IMPLEMENTATION
#define BAD_SYSTICK_SYSTICK_ISR_IMPLEMENTATION
#endif
#include "badhal.h"

//CONFIG
//uncoment those to enable desired functionality
#define BAD_RTOS_USE_KHEAP      //kernel heap
//#define KMIN_ORDER 5          //kernel heap minimal order of allocation (size = 1 << MIN_ORDER = 32)
//#define KMAX_ORDER 12         //kernel heap maximum order of allocation (heap_size) (size = 1 << MIN_ORDER = 4096)
#define BAD_RTOS_USE_UHEAP    //user heap
//#define UMIN_ORDER 5          //user heap minimal order of allocation (size = 1 << MIN_ORDER = 32)
//#define UMAX_ORDER 12         //user heap maximum order of allocation (heap size) (size = 1 << MIN_ORDER = 4096)
#define BAD_RTOS_USE_MUTEX      //mutexes
#define BAD_RTOS_USE_MSGQ       // message queues
#define BAD_RTOS_USE_SEMAPHORE  //semaphores
#define BAD_RTOS_USE_MPU        //mpu
#define BAD_RTOS_MAX_TASKS 32 //maximum number of running tasks


#if BAD_RTOS_MAX_TASKS < 2
    #error "Number of tasks must be > 1 to accomodate for idle task"
#else 
#if BAD_RTOS_MAX_TASKS > 32
    #error "Number of tasks must be <=32"
#endif
#endif

// asm helpers implemented in the asm portion of this file grep for "ASM stuff" to find it
extern uint32_t __modify_basepri(uint32_t basepri);
extern void __restore_basepri(uint32_t basepri);
// critical section helpers using basepri 
#define CONTEX_SWITCH_BARIER_STORE uint32_t context_basepri = 0;
#define CONTEX_SWITCH_BARIER context_basepri = __modify_basepri(15<<4)
#define CONTEX_SWITCH_BARIER_RELEASE __restore_basepri(context_basepri)
#define CRITICAL_SECTION_STORE uint32_t critical_basepri = 0;
#define CRITICAL_SECTION critical_basepri = __modify_basepri(2<<4)
#define CRITICAL_SECTION_END __restore_basepri(critical_basepri)
// error codes 
typedef enum {
    BAD_RTOS_STATUS_ALLOC_FAIL = 0, // to return as null ptr
    BAD_RTOS_STATUS_OK,
    BAD_RTOS_STATUS_BAD_PARAMETERS,
    BAD_RTOS_STATUS_NOT_BLOCKED,
    BAD_RTOS_STATUS_NOT_DELAYED,
    BAD_RTOS_STATUS_WOULD_BLOCK,
    BAD_RTOS_STATUS_CANT_YEILD,
    BAD_RTOS_STATUS_CANT_FINISH,
    BAD_RTOS_STATUS_TIMEOUT,
    BAD_RTOS_STATUS_WRONG_Q,
    BAD_RTOS_STATUS_NOT_OWNER,
    BAD_RTOS_STATUS_RECURSIVE_TAKE,
    BAD_RTOS_STATUS_RECURSIVE_PUT,
    BAD_RTOS_STATUS_WOKEN,
    BAD_RTOS_STATUS_DELETED,
    BAD_RTOS_STATUS_CANT_DELETE,
    BAD_RTOS_STATUS_NOT_INITIALISED,
    BAD_RTOS_STATUS_WRONG_CONTEXT
}bad_rtos_status_t;
// helper enum to discriminate the position of the tcb in a queue
// the logic behind it is
// member enum = head enum + 1 
// this allows to easily check if a tcb is a member of the respective queue or not
typedef enum {
    BAD_RTOS_MISC_RUNNING,
    BAD_RTOS_MISC_READYQ_HEAD,
    BAD_RTOS_MISC_READYQ_MEMBER,
    BAD_RTOS_MISC_BLOCKEDQ_HEAD,
    BAD_RTOS_MISC_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_MUTEX_BLOCKEDQ_HEAD,
    BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_SEM_BLOCKEDQ_HEAD,
    BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER
} bad_rtos_misc_t;
// helper enum for software timer queue
// folows the same logic as the enum above
typedef enum{
    BAD_RTOS_MISC_NOT_DELAYED,
    BAD_RTOS_MISC_DELAYQ_HEAD,
    BAD_RTOS_MISC_DELAYQ_MEMBER
}bad_rtos_delayq_misc_t;

#ifdef BAD_RTOS_USE_MPU
//mpu region struct, stores the precomputed rasr and the adress of the region
typedef struct {
    uint32_t addr;
    uint32_t rasr;
}mpu_region_t;
#endif

typedef void (*taskptr)(void * par) ;
// main fat struct of the program
typedef struct bad_tcb{
    // stack pointer, doesnt really reflect the actual one when running, actual one is + 32
    // (due to registers stacked by hardware), used only to save it for a context switch     
    uint32_t* sp;
    //stack base
    uint32_t* stack;
    uint32_t stack_size;
    taskptr entry;
    //callback for delays
    void (*cbptr)(struct bad_tcb* tcb,void * par);
    //args for the callback
    void* args;
#if defined (BAD_RTOS_USE_KHEAP)
    //a flag for dynamic stack
    uint8_t dyn_stack ;
#endif
    //execution priority, follows nvic logic : lower number is higher priority
    uint8_t base_priority;
    uint8_t raised_priority;
#ifdef BAD_RTOS_USE_MUTEX
    uint8_t mutex_count;
#endif
#if defined (BAD_RTOS_USE_MPU)
    uint8_t region_count;

    const mpu_region_t *regions;
#endif
    struct bad_tcb* prev;
    struct bad_tcb* next;
    bad_rtos_misc_t misc;
    struct bad_tcb* delayq_prev;
    struct bad_tcb* delayq_next;
    bad_rtos_delayq_misc_t delayq_misc;
    uint32_t ticks_to_change;   
    uint32_t counter;
}bad_tcb_t;

typedef void (*cbptr)(struct bad_tcb* tcb,void * par); 
//task decription for task creation
typedef struct{
    
    uint32_t stack_size;
    uint32_t* stack;
    taskptr entry;
    void* args;
#if defined (BAD_RTOS_USE_KHEAP)
    uint8_t dyn_stack;
#endif
#ifdef BAD_RTOS_USE_MPU
    const mpu_region_t *regions;
    uint8_t region_count;
#endif
    uint8_t base_priority;
    uint32_t ticks_to_change;
}bad_task_descr_t;

#ifdef BAD_RTOS_USE_MUTEX
typedef struct bad_mutex{
    bad_tcb_t* owner;
    bad_tcb_t* blockedq;
} bad_mutex_t ;
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
typedef struct bad_sem{
    uint32_t counter;
    uint32_t count;
    bad_tcb_t* blockedq;
} bad_sem_t ;

typedef struct bad_nbsem{
    uint32_t counter;
    uint32_t count;
    
} bad_nbsem_t ;

#endif

#ifdef BAD_RTOS_USE_MSGQ
typedef struct {
    uint32_t signal;
    void* args;
} bad_msg_block_t;

typedef struct {
    bad_msg_block_t* msgs;
    uint32_t head;
    uint32_t tail;
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
 *      DEFINE_PERIPH_ACCESS_REGION(USART1_BASE, sizeof(USART_typedef_t))
 * END_TASK_MPU_REGIONS(task1)
 * Then in task creation:
 *    bad_task_descr_t task1_descr = {
 *     .stack = 0,
 *      .stack_size = TASK1_STACK_SIZE,
 *      .dyn_stack = 1,
 *      .entry = task1,
 *      .regions = task1_regions,
 *      .region_count = MPU_REGIONS_SIZE(task1),
 *      .ticks_to_change = 500,
 *      .base_priority = TASK2_PRIORITY
 *  };
 */

#define START_TASK_MPU_REGIONS_DEFINITIONS(name)\
static const mpu_region_t name##_regions[]={

#define DEFINE_GENERIC_REGION(address,size,tex_scb,ap)\
{.addr = address,.rasr = ap|tex_scb|FIND_SIZE(NEXT_POW2(size))|0x1},

#define DEFINE_PERIPH_ACCESS_REGION(address,size) \
{.addr = address,.rasr = MPU_RASR_XN|MPU_AP_FULL_ACCESS|MPU_TEXSCB_SHARED_DEVICE|FIND_SIZE(NEXT_POW2(size))|0x1},

#define MPU_REGIONS_SIZE(name)\
sizeof(name##_regions)/sizeof(mpu_region_t)

#define END_TASK_MPU_REGIONS(name) \
};\
_Static_assert(MPU_REGIONS_SIZE(name)<=3,"The number of arbitrary regions should be <= 3");
#endif 
//Macro for static stack definition
#ifdef BAD_RTOS_USE_MPU
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert((size&(size-1))==0,"Stack sizes must be powers of 2");\
_Static_assert(size >= 128,"Stacks must be at least 128 bytes to accomodate exception stacked registers and stack cannary");\
static uint32_t task_name##_stack[size/sizeof(uint32_t)] __attribute__((section(".static_stacks")));
#else 
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert(size >= 64,"Stacks must be at least 64 bytes to accomodate exception stacked registers");\
static uint32_t task_name##_stack[size/sizeof(uint32_t)] __attribute__((section(".static_stacks")));

#endif 

// PUBLIC API**********************************************
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
 * @retval  bad_tcb_t* Pointer to a created task
 * @retval  NULL pointer (BAD_RTOS_ALLOC_FAIL) if tcb or stack allocation failed
 */
extern bad_tcb_t* task_make(bad_task_descr_t* descr);

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
 * Delays can be canceled using task_delay_cancel, which would return BAD_RTOS_WOKEN to the specified task using
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
extern bad_rtos_status_t task_delay(uint32_t delay, cbptr cb, void* args );
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
 * If the task is not in blocked list(depending on the misc field) returns BAD_RTOS_NOT_BLOCKED
 *
 * Tasks are unblocked using task_unblock() public function
 *
 * This function can be called from interrupt context.
 * @param[in] bad_tcb_t* Pointer to tcb of the task to unblock
 *
 * @retval  BAD_RTOS_STATUS_OK task is successfully unblocked
 * @retval  BAD_RTOS_STATUS_NOT_BLOCKED the task is not blocked
 */
extern bad_rtos_status_t task_unblock(bad_tcb_t* task);
/**
 * \b task_yield
 *
 * Public SVC (svc 0xD0) call that calls internal function __task_yield
 * Tries to yield to a same priority task
 *
 * 
 * If succedes enqueues current task into an ordered by priotity kernel list of ready tasks as the last task 
 * of the same priority 
 * Then switches context to the highest priority ready task
 * 
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
 * @param[in] bad_tcb_t* Ptr to task tcb to wake up from delay  
 *
 * @retval  BAD_RTOS_STATUS_OK tasks delay successfully canceled
 * @retval  BAD_RTOS_STATUS_NOT_DELAYED task is not delayed 
 */
extern bad_rtos_status_t task_delay_cancel(bad_tcb_t* task);

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
extern bad_rtos_status_t mutex_init(bad_mutex_t* mut);
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
extern bad_rtos_status_t mutex_take(bad_mutex_t*,uint32_t delay);
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
extern bad_rtos_status_t mutex_put(bad_mutex_t* mut);
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
extern bad_rtos_status_t mutex_delete(bad_mutex_t* mut);
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
// Blocking semaphore api 
/**
 * \b sem_init
 *
 * Public function to initialise semaphore object
 * initialises count field to the specifed count
 *
 * No need to call this if you can use an initiliser like bad_sem_t sem = {.count = N }
 *
 * Masks context switch and systick interrupts
 *
 * This function can be called from interrupt context. But is not reentrant if the object parameter is the same
 * @param[in] bad_sem_t* Ptr to semaphore object to initialise
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore ptr is null or count is 0
 */
extern bad_rtos_status_t sem_init(bad_sem_t* sem,uint32_t count);
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
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED sem count is 0
 * @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
 */
extern bad_rtos_status_t sem_take(bad_sem_t*,uint32_t delay);
/**
 * \b sem_put
 *
 * Public SVC (svc 0x88) call that calls internal function __sem_put
 * Tries to put the semaphore
 *
 * If the semaphores counter is less than its recorded count then the highest priority blocked task 
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
 * @retval BAD_RTOS_STATUS_RECURSIVE_PUT the caller tried to increase the sem counter higher than the count 
 */
extern bad_rtos_status_t sem_put(bad_sem_t* sem);
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
 */
extern bad_rtos_status_t sem_delete(bad_sem_t* sem);
//Non blocking semaphore api
/**
 * \b nbsem_init
 *
 * Public function to initialise non blocking semaphore object
 * initialises count field to the specifed count
 *
 * No need to call this if you can use an initiliser like bad_nbsem_t sem = {.count = N }
 *
 * Masks context switch and systick interrupts
 *
 * This function can be called from interrupt context. But is not reentrant if the object parameter is the same
 * @param[in] bad_sem_t* Ptr to semaphore object to initialise
 *
 * @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS nbsemaphore ptr is null or count is 0
 */
extern bad_rtos_status_t nbsem_init(bad_nbsem_t* sem,uint32_t count);
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
 * If the function is called from the isr delay value is ignored and treated as -1
 *
 * This function can be called from interrupt context. This function is reentrant 
 *
 * @param[in] bad_sem_t* Ptr to semaphore object to try take  
 * @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
 *
 * @retval BAD_RTOS_STATUS_OK Semaphore successfully taken
 * @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
 * @retval BAD_RTOS_STATUS_NOT_INITIALISED sem count is 0
 */
extern bad_rtos_status_t nbsem_take(bad_nbsem_t* sem);
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
 * @retval BAD_RTOS_STATUS_RECURSIVE_PUT the caller tried to increase the sem counter higher than the count 
 */
extern bad_rtos_status_t nbsem_put(bad_nbsem_t* sem);
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
 */
extern bad_rtos_status_t nbsem_delete(bad_nbsem_t* sem);
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
extern bad_rtos_status_t msgq_init(bad_msgq_t* q ,uint32_t size, void* (* alloc_func)(uint32_t size));
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
extern bad_rtos_status_t msgq_deinit(bad_msgq_t *q,void(* free_func)(void* block,uint32_t size));
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
extern bad_queue_status_t msgq_pull_msg(bad_msgq_t* q, bad_msg_block_t* writeback );
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
extern bad_queue_status_t msgq_post_msg(bad_msgq_t* q, uint32_t signal, void* args);
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
extern void kernel_free(void*,uint32_t size);
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
 *  @param[in] uint32_t size in bytes 
 * 
 * @retval void * to allocated memory
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL (null ptr) allocation failed 
 */
extern void user_free(void *block, uint32_t size);
/**
 * \b user_free
 *
 * Public function that tries to allocate a specifed number of bytes from user heap
 * Tries to free a specifed number of bytes allocated from user heap
 * 
 * Uses buddy allocator under the hood
 *
 * This function cannot be called from interrupt context. Will fail silently if done so
 * @param[in] uint32_t size in bytes 
 * 
 * @retval void * to allocated memory
 * @retval BAD_RTOS_STATUS_ALLOC_FAIL (null ptr) allocation failed 
 */
extern void* user_alloc(uint32_t size);
#endif

//****************************************************************
#ifdef BAD_RTOS_IMPLEMENTATION

typedef struct kernel_cb {
    bad_tcb_t* readyq;
    bad_tcb_t* blockedq;
    bad_tcb_t* delayq;
    uint8_t is_running;
}bad_kernel_cb_t;

typedef struct bitmask_slab_cb{
    uint32_t free_bitmask;
    uint8_t capacity;
    bad_tcb_t node_arr[BAD_RTOS_MAX_TASKS];
} tcb_bitmask_slab_t;

typedef enum {
    BAD_SYSTICK_TIMEFRAME_PENDING = 0x1,
    BAD_SYSTICK_DELAY_WAKE_PENDING = 0x2,
    BAD_SYSTICK_BOTH = BAD_SYSTICK_DELAY_WAKE_PENDING|BAD_SYSTICK_TIMEFRAME_PENDING//0x3
}BAD_SYSTICK_STATUS;

bad_tcb_t __attribute__((section(".kernel_bss"))) *curr ;
bad_tcb_t __attribute__((section(".kernel_bss"))) *next;

static bad_kernel_cb_t __attribute__((section(".kernel_bss"))) kernel_cb;
static uint32_t __attribute__((section(".kernel_bss")))ticks;

static tcb_bitmask_slab_t __attribute__((section(".kernel_bss"))) tcbslab;

#if defined (BAD_RTOS_USE_UHEAP) || defined (BAD_RTOS_USE_KHEAP)

typedef struct free_block {
    struct free_block* next;
}free_block_t;

typedef struct {
    uint8_t* heap;
    uint32_t max_order;
    uint32_t min_order;
    uint32_t freelist_size;
    free_block_t** free_list;
}buddy_t;

#endif

#if defined (BAD_RTOS_USE_KHEAP) 

#ifndef KMIN_ORDER
    #define KMIN_ORDER 5
#else 
    #if KMIN_ORDER < 2 
        #error "minimal order should be >= 2 to store the pointer to the next free block "
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
static buddy_t __attribute__((section(".kernel_bss")))kernel_buddy;
static free_block_t* __attribute__((section(".kernel_bss"))) kfreelist[KFREE_LIST_SIZE];
#endif

#if defined (BAD_RTOS_USE_UHEAP)

#ifndef UMIN_ORDER
    #define UMIN_ORDER 5
#else 
    #if UMIN_ORDER < 2 
        #error "minimal order should be >= 2 to store the pointer to the next free block "
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
static buddy_t user_buddy;
static free_block_t* ufreelist[KFREE_LIST_SIZE];

#endif

#define IDLE_TASK_PRIO 255
#define IDLE_TASK_STACK_SIZE 128

TASK_STATIC_STACK(idle, IDLE_TASK_STACK_SIZE)
// Prototypes for asm helpers, for implementation look right above svc_c function, or grep for "ASM stuff"
extern void idle_task();
extern void __first_task_start();
extern void __sched_update_systick(BAD_SYSTICK_STATUS status);
extern uint32_t __ldrex(void*);
extern uint32_t __strex(uint32_t val,void*);
extern void __clrex();
extern void __set_control(uint32_t control);
extern uint32_t __get_control();
extern void __init_second_stage();

#ifdef BAD_RTOS_USE_MPU
#define STACK_RASR MPU_RASR_ENABLE|(0x4)<<1|MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|MPU_AP_PRIV_RW_UNPRIV_FAULT
/**
 * \b __task_mpu_apply_perms
 *
 * Internal function that applies task specific mpu rules
 * 
 *
 * This function should not be called by the application
 * @param[in] tcb ptr to a task 
 */
ALWAYS_INLINE void __task_mpu_apply_perms(bad_tcb_t* tcb){
    MPU->RNR = 4;
    MPU->RBAR = (uint32_t)tcb->stack;
    MPU->RASR = STACK_RASR;
    switch (tcb->region_count) {
        case 0:{
            return;
        }
        case 3:{
            MPU->RBAR_A3 = tcb->regions[2].addr;
            MPU->RASR_A3 = tcb->regions[2].rasr;
        }
        /*fallthrough*/
        case 2:{
            MPU->RBAR_A2 = tcb->regions[1].addr;
            MPU->RASR_A2 = tcb->regions[1].rasr;
        }
        /*fallthrough*/
        case 1:{
            MPU->RBAR_A1 = tcb->regions[0].addr;
            MPU->RASR_A1 = tcb->regions[0].rasr;
        }
    }
}
//Linker script symbols
extern uint8_t __kernel_bss;
extern uint8_t __static_stacks;
/**
 * \b __bad_rtos_mpu_default_setup
 *
 * Internal function that applies general mpu rules
 * 
 *
 * This function should not be called by the application
 */
ALWAYS_INLINE void __bad_rtos_mpu_default_setup(){
    //ram region
    MPU->RNR = 0;
    MPU->RBAR = 0x20000000;
    MPU->RASR = MPU_RASR_ENABLE|(0x10)<<1|MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|MPU_AP_FULL_ACCESS;
    //flash region
    MPU->RNR = 5;
    MPU->RBAR = 0x08000000;
    MPU->RASR = MPU_RASR_ENABLE|(0x12)<<1|MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|MPU_AP_PRIV_RO_UNPRIV_RO;
    //null adress
    MPU->RNR = 6;
    MPU->RBAR = 0x08000000;
    MPU->RASR = MPU_RASR_ENABLE|(0x4)<<1|MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|MPU_AP_NO_ACCESS;
    //kernel data structures
    MPU->RNR = 7;
    MPU->RBAR = 0x20000000;
    MPU->RASR = MPU_RASR_ENABLE|
        FIND_SIZE(PREV_POW2(&__static_stacks - &__kernel_bss))|
        MPU_AP_PRIV_RW_UNPRIV_FAULT|MPU_RASR_XN;
    mpu_enable_with_default_map();
}
#endif

#ifdef BAD_RTOS_USE_MSGQ
#if defined (BAD_RTOS_USE_UHEAP) || defined (BAD_RTOS_USE_KHEAP)

bad_rtos_status_t msgq_init(bad_msgq_t* q ,uint32_t size, void* (* alloc_func)(uint32_t size)){
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
    free_func(q->msgs,cap);
    q->head = 0;
    q->tail = 0;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}
#endif

bad_queue_status_t msgq_pull_msg(bad_msgq_t* q, bad_msg_block_t* writeback ){
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
    *writeback = *(q->msgs+q->tail);
    q->tail = (q->tail+1) & (q->capacity-1);;

    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_QUEUE_OK;
}


bad_queue_status_t msgq_post_msg(bad_msgq_t* q, uint32_t signal, void* args){
    uint32_t head,next_head;
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    
    if(!q){
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
    
    DMB;
    
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
 * @param[in] buddy_t* buddy allocator control block
 * @param[in] uint8_t * pointer to allocatable memory
 * @param[in] free_block_t** pointer to allocated free list
 * @param[in] uint32_t minimal order of allocation
 * @param[in] uint32_t maximum order of allocation (heap size)
 */
void  __buddy_init(buddy_t* cb,uint8_t* heap, free_block_t** freelist,uint32_t min_order, uint32_t max_order){
    cb->min_order = min_order;
    cb->max_order = max_order;
    cb->heap = heap;
    cb->free_list = freelist;
    *cb->free_list = (free_block_t*)cb->heap;
    free_block_t* embedded_node = (free_block_t*)cb->heap;
    embedded_node->next = 0;

}
/**
 * \b __buddy_alloc
 *
 * Internal function that allocates a block of memory of a specifed order from a specifed buddy control block
 *
 * This function should not be called by the application
 *
 * @param[in] buddy_t* buddy allocator control block
 * @param[in] uint32_t order of allocation
 *
 * @retval void* allocated block
 */
static void* __buddy_alloc(buddy_t *cb,uint32_t order){
    if(order < cb->min_order || order > cb->max_order){
        return 0;
    }
    uint32_t idx = cb->max_order  - order;
    uint32_t picked_idx = idx;
    
    for( ; picked_idx != UINT32_MAX ; picked_idx--){
        if(cb->free_list[picked_idx]!=0){
            break;
        }
    }
    
    if(picked_idx==UINT32_MAX){
        return (void* )BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    
    uint32_t splits = idx - picked_idx;
    uint8_t *block_for_split = (uint8_t*)cb->free_list[picked_idx];
    cb->free_list[picked_idx] = cb->free_list[picked_idx]->next;
    uint32_t splited_block_size = 1 << (cb->max_order - picked_idx-1);
    free_block_t* unused_block = 0;

    for(uint32_t i =0; i < splits;i++){
        unused_block = (free_block_t*)(block_for_split+splited_block_size);
        unused_block->next = cb->free_list[picked_idx+1];
        cb->free_list[picked_idx+1] = unused_block;
        
        picked_idx++;
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
 * @param[in] buddy_t* buddy allocator control block
 * @param[in] void * block to free
 * @param[in] uint32_t order of allocation
 *
 */
static void __buddy_free(buddy_t* cb,void* block,uint32_t order ){
    
    if(order < cb->min_order || order > cb->max_order){
        return;
    }

    uint32_t curr_order = order; 
    uint32_t found_buddy = 0;
    void *curr_block = block;
    uint32_t idx = 0;

    do{
        idx = cb->max_order  - curr_order;
        uint32_t buddy_bitmask = 1ULL << curr_order;
        found_buddy = 0;
        uint32_t offset = (uint8_t*)curr_block - cb->heap;
        uint32_t buddy_offset = offset ^ buddy_bitmask;
        uint32_t parent_offset = offset &(~buddy_bitmask);
        void *buddy_addr = (void*)(cb->heap + buddy_offset);
        void *parent_addr = (void*)(cb->heap + parent_offset);

        free_block_t *traverse = cb->free_list[idx];
        free_block_t *prev = 0;
  
        if(traverse == buddy_addr){
            found_buddy = 1;
            cb->free_list[idx] = cb->free_list[idx]->next;
            curr_block = parent_addr;
            curr_order++;
            continue;
        }

        while (traverse!=0) {
            prev = traverse;
            traverse = traverse->next;
            if(traverse == buddy_addr){
                prev->next = traverse->next;
                curr_block = parent_addr;
                curr_order++;
                found_buddy = 1;
                break;
            }
        } 

    }while (found_buddy);

    free_block_t *final_block = (free_block_t*)curr_block;
    final_block->next = cb->free_list[idx];
    cb->free_list[idx] = final_block;
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

ALWAYS_INLINE void * __kernel_alloc(uint32_t size){
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
ALWAYS_INLINE void __kernel_free(void * block,uint32_t size){
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
    block =    __buddy_alloc(&user_buddy,closest_order );
    CONTEX_SWITCH_BARIER_RELEASE;
    return  block; 

}

void user_free(void* block, uint32_t size){
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
ALWAYS_INLINE void __tcb_queue_slab_init(){
    tcbslab.free_bitmask = (1UL<<(BAD_RTOS_MAX_TASKS))-1;
    tcbslab.capacity = BAD_RTOS_MAX_TASKS;
}
/**
 * \b __tcb_node_alloc
 *
 * Internal function that allocates a node from global tcb bitmask slab
 *
 * This function should not be called by the application
 * @retval bad_tcb_t * ptr to allocated tcb node
 */
ALWAYS_INLINE bad_tcb_t* __tcb_node_alloc(){
    if(tcbslab.free_bitmask == 0){
        return (bad_tcb_t*)BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    uint8_t block_idx = __builtin_ctz(tcbslab.free_bitmask);
    tcbslab.free_bitmask &= ~(1UL<<block_idx);
    return tcbslab.node_arr+block_idx;
    
}
/**
 * \b __tcb_node_free
 *
 * Internal function that frees a node from global tcb bitmask slab
 *
 * This function should not be called by the application
 */
ALWAYS_INLINE void __tcb_node_free(bad_tcb_t* block){

    if(block < tcbslab.node_arr || block >= tcbslab.node_arr+tcbslab.capacity){
        return;
    }
    uint8_t block_idx = block - tcbslab.node_arr;
    tcbslab.free_bitmask |= (1ULL<<block_idx); 
}
/**
 * \b  __tcb_bitmask_slab_clear
 *
 * Internal function that frees all nodes in global tcb bitmask slab, never called 
 *
 * This function should not be called by the application
 */
ALWAYS_INLINE void __tcb_bitmask_slab_clear(){
    tcbslab.free_bitmask = (1ULL<<tcbslab.capacity)-1;
}

/**
 * \b __enqueue_by_prio
 *
 * Internal function that enqueues a specifed tcb into a specifed queue by its priority
 *
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_tcb_t** queue ptr
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] uint32_t HEAD enum value of the specifed queue 
 */
ALWAYS_INLINE void __enqueue_by_prio(bad_tcb_t** queue,bad_tcb_t* tcb,uint32_t base_of_enum){


    if(!*queue){
        *queue = tcb;
        tcb->misc = base_of_enum;
        tcb->next = 0;
        tcb->prev = 0;
        return;
    }
    

    bad_tcb_t* traverse=*queue;
    
    if(traverse->raised_priority > tcb->raised_priority ){
        tcb->next = traverse;
        traverse->prev = tcb;
        tcb->prev = 0;
        tcb->misc = base_of_enum;
        *queue = tcb;
        traverse->misc = base_of_enum+1;
        return;
    }

    while (traverse->raised_priority <= tcb->raised_priority) {
        
        if(traverse->next == 0){
           traverse->next = tcb;
           tcb->prev = traverse;
           tcb->next = 0;
           tcb->misc = base_of_enum+1;
           return;
        }
        traverse = traverse->next;
    }
    
    tcb->next = traverse;
    tcb->prev = traverse->prev;
    tcb->misc = base_of_enum+1;
    traverse->prev = tcb;
    tcb->prev->next = tcb;
}

/**
 * \b __enqueue_delayq
 *
 * Internal function that enqueues a specifed tcb into a kernel.delayq delta list
 * 
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] uint32_t ablsolute delay value 
 */

ALWAYS_INLINE void __delayq_enqueue( bad_tcb_t *tcb,uint32_t absolute ){
    if(!kernel_cb.delayq){
        kernel_cb.delayq = tcb;
        tcb->counter = absolute;
        tcb->delayq_misc = BAD_RTOS_MISC_DELAYQ_HEAD;
        tcb->delayq_next = 0;
        tcb->delayq_prev = 0;
        return;
    }

    bad_tcb_t * traverse = kernel_cb.delayq;
    
    uint32_t compound = 0;
    if(traverse->counter >= absolute){
        traverse->counter -= absolute;
        tcb->delayq_misc = BAD_RTOS_MISC_DELAYQ_HEAD;
        tcb->delayq_next = traverse;
        traverse->delayq_prev = tcb;
        tcb->delayq_prev = 0;
        tcb->counter = absolute;
        traverse->delayq_misc = BAD_RTOS_MISC_DELAYQ_MEMBER;
        kernel_cb.delayq = tcb;
        return;
    }

    while((compound+=traverse->counter) < absolute ){
        if(traverse->delayq_next == 0){
            tcb->delayq_misc = BAD_RTOS_MISC_DELAYQ_MEMBER;    
            tcb->counter = absolute - compound;
            traverse->delayq_next = tcb;
            tcb->delayq_prev = traverse;
            tcb->delayq_next = 0;
            return;
        }

        traverse = traverse->delayq_next;

    }

    compound -= traverse->counter;
    tcb->counter = absolute - compound;
    tcb->delayq_misc = BAD_RTOS_MISC_DELAYQ_MEMBER;
    traverse->counter -= tcb->counter; 
    tcb->delayq_prev = traverse->delayq_prev;
    tcb->delayq_prev->delayq_next = tcb;
    traverse->delayq_prev = tcb;
    tcb->delayq_next = traverse;

}
/**
 * \b __enqueue_delayq
 *
 * Internal function that dequeues a specifed tcb from a kernel.delayq delta list
 * 
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * @param[in] bad_tcb_t* tcb to denqueue
 *
 */
ALWAYS_INLINE bad_rtos_status_t __dequeue_delayq(bad_tcb_t* tcb){
    if(!kernel_cb.delayq){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(tcb->delayq_misc == BAD_RTOS_MISC_NOT_DELAYED){
        return BAD_RTOS_STATUS_WRONG_Q;
    }
    
    if(kernel_cb.delayq == tcb){
        kernel_cb.delayq = kernel_cb.delayq->delayq_next;
        if(kernel_cb.delayq){
            kernel_cb.delayq->counter+=tcb->counter;
            kernel_cb.delayq->delayq_misc = BAD_RTOS_MISC_DELAYQ_HEAD;
        }
    }else {
        tcb->delayq_prev->delayq_next = tcb->delayq_next;
        if(tcb->delayq_next){
            tcb->delayq_next->counter+=tcb->counter;
            tcb->delayq_next->delayq_prev = tcb->delayq_prev;
        }

    }
    tcb->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED;
    return BAD_RTOS_STATUS_OK;
}
/**
 * \b __dequeue_head
 *
 * Internal function that dequeues the head of the specifed queue
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 * @param[in] bad_tcb_t** queue to dequeue head from
 * @param[in] uint32_t HEAD enum value of the specifed queue 
 *
 * @retval bad_tcb_t* pointer to the dequeued head
 */
ALWAYS_INLINE bad_tcb_t * __dequeue_head(bad_tcb_t** q ,uint32_t base_of_enum){
    bad_tcb_t *head = *q;
    if(!head){
        return 0;
    }
    bad_tcb_t *new_head = head->next; 
    *q = new_head;
    if(new_head){
        new_head->misc = base_of_enum;
        new_head->prev = 0;
    }
    return head;
}
/**
 * \b __dequeue_delayq_head
 *
 * Internal function that dequeues the head of the kernel_cb.delayq delta list
 *  
 *
 * This function should not be called by the application
 *
 * @retval bad_tcb_t* pointer to the dequeued head
 */
ALWAYS_INLINE bad_tcb_t * __dequeue_delayq_head(){
    if(!kernel_cb.delayq){
        return 0;
    }
    bad_tcb_t * head = kernel_cb.delayq;
    head->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED;
    kernel_cb.delayq = kernel_cb.delayq->delayq_next; 
    if(kernel_cb.delayq){
        kernel_cb.delayq->delayq_misc = BAD_RTOS_MISC_DELAYQ_HEAD;
        kernel_cb.delayq->delayq_prev = 0;
        //since the only time this function is called is when delay head expires we dont need this for now
        //kernel_cb.delayq->counter+= head->counter;     
    }
    return head;
}
/**
 * \b __remove_head
 *
 * Internal function that removes the head of the specifed queue
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 * @param[in] bad_tcb_t** queue to dequeue head from
 * @param[in] uint32_t HEAD enum value of the specifed queue 
 *
 */

ALWAYS_INLINE void __remove_head(bad_tcb_t** queue ,uint32_t base_of_enum){
    bad_tcb_t *head = *queue;
    if(!head){
        return ;
    }
    bad_tcb_t *new_head = head->next; 
    *queue = new_head;
    if(new_head){
        new_head->misc = base_of_enum;
        new_head->prev = 0;
    }
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
 * @param[in] bad_tcb_t** queue to dequeue head from
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] uint32_t HEAD enum value of the specifed queue 
 *
 */
ALWAYS_INLINE void __enqueue_head(bad_tcb_t** q, bad_tcb_t * tcb,uint32_t base_of_enum){
    bad_tcb_t * old_head = *q;
    if(old_head){ 
        old_head->prev = tcb;
        old_head->misc = base_of_enum+1; 
    }

    tcb->next = old_head;
    tcb->prev = 0;
    tcb->misc = base_of_enum;
    *q = tcb;
}
/**
 * \b __remove_member
 *
 * Internal function that removes a member of the queue, should not be called if the tcb is a queues head
 *   
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 *
 * @param[in] bad_tcb_t* tcb to remove
 */
ALWAYS_INLINE void __remove_member(bad_tcb_t* tcb){
    tcb->prev->next = tcb->next;
    if(tcb->next){
        tcb->next->prev = tcb->prev;
    }
}
/**
 * \b __remove_entry
 *
 * Internal function that removes a tcb from a specified queue depending on is it a queues head or not will 
 * call the fucntions defined above. If its not in the specified queue (depending on the enum provided) will return
 * BAD_RTOS_STATUS_WRONG_Q
 *   
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * This function should not be called on kernel_cb.delayq delta list
 *
 * @param[in] bad_tcb_t** queue to dequeue head from
 * @param[in] bad_tcb_t* tcb to enqueue
 * @param[in] uint32_t HEAD enum value of the specifed queue
 *
 * @retval BAD_RTOS_STATUS_OK successfully removed the tcb from the queue
 * @retval BAD_RTOS_STATUS_WRONG_Q tcb is not in the specified queue
 */
ALWAYS_INLINE bad_rtos_status_t __remove_entry(bad_tcb_t**q ,bad_tcb_t* tcb,uint32_t base_of_enum){
    uint32_t offset_from_base = tcb->misc - base_of_enum;
    switch (offset_from_base) {
        case 0:{
            __remove_head(q,base_of_enum);
            break;
        }
        case 1:{
            __remove_member(tcb);
            break;
        }
        default:
            return BAD_RTOS_STATUS_WRONG_Q;
            break;
    }
    return BAD_RTOS_STATUS_OK;
}
/**
 * \b __sched_update
 *
 * Internal function that schedules a context switch
 * Calls __task_mpu_apply_perms if mpu is enabled
 *  
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * The tcb running should not be in any queue
 * @param[in] bad_tcb_t* tcb to run
 *
 */
ALWAYS_INLINE void __sched_update(bad_tcb_t* tcb){
    next = tcb;
    tcb->misc = BAD_RTOS_MISC_RUNNING;
#ifdef BAD_RTOS_USE_MPU
    __task_mpu_apply_perms(tcb);
#endif
    if(!tcb->counter){
        tcb->counter = tcb->ticks_to_change;
    }
    SCB->ICSR = SCB_ICSR_PENDSVSET;
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
ALWAYS_INLINE void __sched_try_update(){
    if(next){
        if(kernel_cb.readyq->raised_priority < next->raised_priority){
            __enqueue_by_prio(&kernel_cb.readyq, next, BAD_RTOS_MISC_READYQ_HEAD);
            __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
        }
        return;
    }

    if(kernel_cb.readyq->raised_priority < curr->raised_priority ){
        __enqueue_by_prio(&kernel_cb.readyq,curr,BAD_RTOS_MISC_READYQ_HEAD);
        __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
    }
}
/**
 * \b __sched_try_preempt
 *
 * Internal function that check if a specified task that should preempt the current one or an already scheduled one
 * 
 * Will derefence a null ptr if passed
 *
 * This function should not be called by the application
 * The tcb running should not be in any queue
 *
 */
ALWAYS_INLINE void __sched_try_preempt(bad_tcb_t* tcb){
    if(next){
        if(tcb->raised_priority < next->raised_priority){
            __enqueue_by_prio(&kernel_cb.readyq, next, BAD_RTOS_MISC_READYQ_HEAD);
            __sched_update(tcb);
        }else {
            __enqueue_by_prio(&kernel_cb.readyq, tcb, BAD_RTOS_MISC_READYQ_HEAD);
        }
        return;
    }

    if(tcb->raised_priority < curr->raised_priority){
        __enqueue_by_prio(&kernel_cb.readyq, curr, BAD_RTOS_MISC_READYQ_HEAD);
        __sched_update(tcb);
    }else {
        __enqueue_by_prio(&kernel_cb.readyq, tcb, BAD_RTOS_MISC_READYQ_HEAD);
    }

}

ALWAYS_INLINE void  __task_block(){
    __enqueue_head(&kernel_cb.blockedq, curr,BAD_RTOS_MISC_BLOCKEDQ_HEAD);
    __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
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
ALWAYS_INLINE void __handle_systick_event(BAD_SYSTICK_STATUS status){

    switch (status) {

        case BAD_SYSTICK_DELAY_WAKE_PENDING:{
            do{ 
                bad_tcb_t* wake = __dequeue_delayq_head();
                if(wake->cbptr){
                    wake->cbptr(wake,wake->args);
                    wake->cbptr = 0;
                    wake->args = 0;
                }
                wake->counter = wake->ticks_to_change;
                __enqueue_by_prio(&kernel_cb.readyq,wake,BAD_RTOS_MISC_READYQ_HEAD);
            }while(kernel_cb.delayq && !kernel_cb.delayq->counter);
            __sched_try_update();
            break;
        }
        case BAD_SYSTICK_BOTH:{
            do{ 
                bad_tcb_t* wake = __dequeue_delayq_head();
                if(wake->cbptr){
                    wake->cbptr(wake,wake->args);
                    wake->cbptr = 0;
                    wake->args = 0;
                }
                wake->counter = wake->ticks_to_change;
                __enqueue_by_prio(&kernel_cb.readyq,wake,BAD_RTOS_MISC_READYQ_HEAD);
            }while(kernel_cb.delayq && !kernel_cb.delayq->counter);

        }
        /* fallthrough */
        case BAD_SYSTICK_TIMEFRAME_PENDING:{
            if(next){//todo check if this is needed
                if(kernel_cb.readyq->raised_priority < next->raised_priority){
                    __enqueue_by_prio(&kernel_cb.readyq, next, BAD_RTOS_MISC_READYQ_HEAD);
                    __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
                }
                return;
            }

            if(kernel_cb.readyq&&(kernel_cb.readyq->raised_priority <= curr->raised_priority)){
                __enqueue_by_prio(&kernel_cb.readyq,curr,BAD_RTOS_MISC_READYQ_HEAD);
                __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
            }else{
                curr->counter = curr->ticks_to_change;
            }
            break;
        }
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
ALWAYS_INLINE uint32_t * __init_stack(void (*task)(), uint32_t* stacktop,void * args){
    *--stacktop = 0x01000000UL;     // xPSR (Thumb bit set)
    *--stacktop = (uint32_t)task|0x1;   // PC
    *--stacktop = 0xFFFFFFFDUL;     // LR = EXC_RETURN (thread mode + PSP)
    *--stacktop = 0x12121212UL;     // R12
    *--stacktop = 0x03030303UL;     // R3
    *--stacktop = 0x02020202UL;     // R2
    *--stacktop = 0x01010101UL;     // R1
    *--stacktop = (uint32_t)args;     // R0 (parameter, optional)
    for(uint8_t i = 0;i<8;i++){
        *--stacktop = 0xDEADBEEFUL;
    }
    return stacktop;
}


ALWAYS_INLINE bad_tcb_t* __task_make(bad_task_descr_t* args){
    bad_tcb_t* new_task = __tcb_node_alloc();
    if(!new_task){
        return (bad_tcb_t*)BAD_RTOS_STATUS_ALLOC_FAIL; //shut up warning, its zero
    }
    new_task->entry = args->entry;
    new_task->base_priority = args->base_priority;
    new_task->raised_priority = args->base_priority;
#if defined (BAD_RTOS_USE_KHEAP)
    new_task->dyn_stack = args->dyn_stack;
    if(args->dyn_stack){
#ifdef BAD_RTOS_USE_MPU
        if(args->stack_size < 64){
             return (bad_tcb_t*)BAD_RTOS_STATUS_ALLOC_FAIL;
        }
#endif

        new_task->stack = __kernel_alloc(args->stack_size);
    }else{
        new_task->stack = args->stack;
    }

#else
    new_task->stack = args->stack;
#endif
#ifdef BAD_RTOS_USE_MPU
    new_task->regions = args->regions;
    new_task->region_count = args->region_count;
#endif 
    new_task->stack_size = args->stack_size;
    new_task->ticks_to_change = args->ticks_to_change;
    new_task->counter = args->ticks_to_change;
    new_task->sp = __init_stack(new_task->entry, new_task->stack + (args->stack_size/sizeof(uint32_t)),args->args);
    if(kernel_cb.is_running ){
        __sched_try_preempt(new_task);
    }else{
        __enqueue_by_prio(&kernel_cb.readyq, new_task,BAD_RTOS_MISC_READYQ_HEAD);
    }
    return new_task;
}



ALWAYS_INLINE bad_rtos_status_t __task_unblock(bad_tcb_t* tcb){
    if(__remove_entry(&kernel_cb.blockedq, tcb, BAD_RTOS_MISC_BLOCKEDQ_HEAD)!= BAD_RTOS_STATUS_OK){
        return BAD_RTOS_STATUS_NOT_BLOCKED;
    }
    __sched_try_preempt(tcb);
    return BAD_RTOS_STATUS_OK;
}

ALWAYS_INLINE bad_rtos_status_t __task_delay_cancel(bad_tcb_t* tcb){
    if(__dequeue_delayq(tcb)!= BAD_RTOS_STATUS_OK){
        return BAD_RTOS_STATUS_NOT_DELAYED;
    }

    tcb->cbptr = 0;
    tcb->args = 0;
    *(tcb->sp+8) = BAD_RTOS_STATUS_WOKEN;
    __sched_try_preempt(tcb);
    return BAD_RTOS_STATUS_OK;
}

ALWAYS_INLINE bad_rtos_status_t __task_yield(){
    if(kernel_cb.readyq->raised_priority == curr->raised_priority){
        __enqueue_by_prio(&kernel_cb.readyq, curr, BAD_RTOS_MISC_READYQ_HEAD);
        __sched_update(__dequeue_head(&kernel_cb.readyq, BAD_RTOS_MISC_READYQ_HEAD));
        return BAD_RTOS_STATUS_OK;
    }
    else{
        return BAD_RTOS_STATUS_CANT_YEILD;
    }
}

#ifdef BAD_RTOS_USE_MUTEX
bad_rtos_status_t mutex_init(bad_mutex_t* mut){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!mut){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    mut->blockedq = 0;
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
ALWAYS_INLINE void __mutex_timeout_cb(bad_tcb_t* tcb ,void * mutex){
    bad_mutex_t * mut = (bad_mutex_t* ) mutex;
    __remove_entry(&mut->blockedq,tcb,BAD_RTOS_MISC_MUTEX_BLOCKEDQ_HEAD);
    *(tcb->sp+8)=BAD_RTOS_STATUS_TIMEOUT;
}

ALWAYS_INLINE bad_rtos_status_t __mutex_delete(bad_mutex_t* mut){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!mut->owner || curr != mut->owner ){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    bad_tcb_t * tcb;
    while((tcb = __dequeue_head(&mut->blockedq, BAD_RTOS_MISC_MUTEX_BLOCKEDQ_HEAD))){
        *(tcb->sp+8) = BAD_RTOS_STATUS_DELETED;
        __enqueue_by_prio(&kernel_cb.readyq, tcb, BAD_RTOS_MISC_READYQ_HEAD);
        if(tcb->cbptr == __mutex_timeout_cb){
            tcb->cbptr = 0;
            tcb->args = 0;
            __dequeue_delayq(tcb);
        }
    }

    curr->mutex_count--;
    if(!curr->mutex_count){
        curr->raised_priority = curr->base_priority;
    }
    mut->owner = 0;
    __sched_try_update();

    return BAD_RTOS_STATUS_OK;
}

ALWAYS_INLINE bad_rtos_status_t __mutex_take(bad_mutex_t* mut, uint32_t delay){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!mut->owner){
        mut->owner = curr;
        curr->mutex_count++;
        return BAD_RTOS_STATUS_OK;
    }
    if(mut->owner == curr){
        return BAD_RTOS_STATUS_RECURSIVE_TAKE;
    }
    if(delay == UINT32_MAX){
        return BAD_RTOS_STATUS_WOULD_BLOCK;
    }

    if(delay){
        curr->args = mut;
        curr->cbptr = __mutex_timeout_cb;
        __delayq_enqueue( curr, delay);
    }


    __enqueue_by_prio(&mut->blockedq,curr, BAD_RTOS_MISC_MUTEX_BLOCKEDQ_HEAD);
    __sched_update(__dequeue_head(&kernel_cb.readyq ,BAD_RTOS_MISC_READYQ_HEAD));
    if(curr->raised_priority < mut->owner->raised_priority){
        mut->owner->raised_priority = curr->raised_priority; 
    }


    return BAD_RTOS_STATUS_OK;
}   


ALWAYS_INLINE bad_rtos_status_t __mutex_put(bad_mutex_t* mut){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(curr!= mut->owner){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }

    mut->owner = __dequeue_head(&mut->blockedq ,BAD_RTOS_MISC_MUTEX_BLOCKEDQ_HEAD);
    mut->owner->mutex_count++;
    curr->mutex_count--;
    if(!curr->mutex_count){
        curr->raised_priority = curr->base_priority;
    }

    if(!mut->owner){ 
        return BAD_RTOS_STATUS_OK; 
    }

    if(mut->owner->cbptr == __mutex_timeout_cb){
            mut->owner->cbptr = 0;
            mut->owner->args = 0;
            __dequeue_delayq(mut->owner);
    }
         
    *(mut->owner->sp+8) = BAD_RTOS_STATUS_OK;
    
    __sched_try_preempt(mut->owner);

    return BAD_RTOS_STATUS_OK;
}
#endif


#ifdef BAD_RTOS_USE_SEMAPHORE
bad_rtos_status_t sem_init(bad_sem_t* sem,uint32_t count){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!sem || !count){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    sem->blockedq = 0;
    sem->counter = count;
    sem->count = count;
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
ALWAYS_INLINE void __sem_timeout_cb(bad_tcb_t* tcb ,void * semaphore){
    bad_sem_t * sem = (bad_sem_t* ) semaphore;
    __remove_entry(&sem->blockedq,tcb,BAD_RTOS_MISC_SEM_BLOCKEDQ_HEAD);
    *(tcb->sp+8)=BAD_RTOS_STATUS_TIMEOUT;
}

ALWAYS_INLINE bad_rtos_status_t __sem_delete(bad_sem_t* sem){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(sem->counter != sem->count){
        return BAD_RTOS_STATUS_CANT_DELETE;
    }
    sem->counter = 0;
    sem->count = 0; 
    __sched_try_update();

    return BAD_RTOS_STATUS_OK;
}

ALWAYS_INLINE bad_rtos_status_t __sem_take(bad_sem_t* sem, uint32_t delay){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!sem->count){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    if(!sem->counter){
        if(delay == UINT32_MAX){
            return BAD_RTOS_STATUS_WOULD_BLOCK;
        }

        if(delay){
            curr->args = sem;
            curr->cbptr = __sem_timeout_cb;
            __delayq_enqueue( curr, delay);
        }
        __enqueue_by_prio(&sem->blockedq,curr, BAD_RTOS_MISC_SEM_BLOCKEDQ_HEAD);
        __sched_update(__dequeue_head(&kernel_cb.readyq ,BAD_RTOS_MISC_READYQ_HEAD));
        return BAD_RTOS_STATUS_OK;
    }
    sem->counter--;

    return BAD_RTOS_STATUS_OK;
}   


ALWAYS_INLINE bad_rtos_status_t __sem_put(bad_sem_t* sem){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }

    if(!sem->count){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }

    if(sem->count == sem->counter){
        return BAD_RTOS_STATUS_RECURSIVE_PUT;
    }
    // wakes a task and returns a success code using stacked registers
    if(sem->blockedq){
        bad_tcb_t * tcb = __dequeue_head(&sem->blockedq, BAD_RTOS_MISC_SEM_BLOCKEDQ_HEAD);
        if(tcb->cbptr == __sem_timeout_cb){
            __dequeue_delayq(tcb);
            tcb->cbptr = 0;
            tcb->args = 0;
        }
        *(tcb->sp+8) = BAD_RTOS_STATUS_OK;
        __sched_try_preempt(tcb);
        return BAD_RTOS_STATUS_OK;
    }

    sem->counter++;
    return BAD_RTOS_STATUS_OK;
}
// you dont need to call this if you initialise it yourself on .data 
bad_rtos_status_t nbsem_init(bad_nbsem_t * nbsem,uint32_t count){ 
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem || !count){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    nbsem->counter = count;
    nbsem->count = count;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t nbsem_take(bad_nbsem_t* nbsem){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
 
    if(!nbsem->count){
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

    DMB;

    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t nbsem_put(bad_nbsem_t* nbsem){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }

    if(!nbsem->count){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }

    uint32_t counter;
    do{
        counter = __ldrex(&nbsem->counter);
        if(counter == nbsem->count){
            __clrex();
            CONTEX_SWITCH_BARIER_RELEASE;
            return BAD_RTOS_STATUS_RECURSIVE_PUT;
        }
    }while(__strex(counter+1, &nbsem->counter));

    DMB;

    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t nbsem_delete(bad_nbsem_t * nbsem){
    CONTEX_SWITCH_BARIER_STORE;
    CONTEX_SWITCH_BARIER;
    if(!nbsem){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(nbsem->counter != nbsem->count){
        CONTEX_SWITCH_BARIER_RELEASE;
        return BAD_RTOS_STATUS_CANT_DELETE;
    }
    //basically just resets the semaphore, leaving memory operations to the program
    nbsem->counter = 0;
    nbsem->count = 0;
    CONTEX_SWITCH_BARIER_RELEASE;
    return BAD_RTOS_STATUS_OK;
}

#endif

ALWAYS_INLINE void __task_finish(){
#ifdef BAD_RTOS_USE_MUTEX
    if(curr->mutex_count){ //trap when task want to finish without releasing mutexes
        __builtin_trap();
        return;
    }
#endif 
#if defined (BAD_RTOS_USE_KHEAP)
    if(curr->dyn_stack){ //free the dynamically allocated stack 
        __kernel_free((void*)curr->stack,curr->stack_size);
    }
#endif
    if(!next){
        __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
    }
    __tcb_node_free(curr);//free the the tcb used by task
}

ALWAYS_INLINE void __task_delay(uint32_t delay,cbptr cb, void* args){
    curr->cbptr =cb;
    curr->args = args;
    __delayq_enqueue(curr,delay);
    __sched_update(__dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD));
   
}

// ASM stuff

//idle task
__asm__(
".thumb_func        \n"
".global idle_task  \n"
"idle_task:         \n"
"infinite_loop:     \n"
    "dsb            \n"
    "wfi            \n"
    "b infinite_loop\n"
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

__asm__(
".thumb_func        \n"
".global task_yield \n"
"task_yield:        \n"
    "svc 0xD0       \n"
    "bx lr          \n"
);

__asm__(
".thumb_func        \n"
".global block_task \n"
"task_block:        \n"
    "svc 0xE0       \n"
    "bx lr          \n"
);

__asm__(
".thumb_func                    \n"
".global __sched_update_systick \n"
"__sched_update_systick:        \n"
    "svc 0xF                    \n"
    "bx lr                      \n"
);

__asm__(
".thumb_func        \n"
".global task_delay \n"
"task_delay:        \n"
    "svc 0xF0       \n"
    "bx lr          \n"
);
// svc isr ,masks lower nibble if the call was from thread context and the higher one if the call was from isr
__asm__ (
".thumb_func            \n"
".global svc_isr        \n"
"svc_isr:               \n"
    "tst     lr, #4     \n"
    "beq     isr        \n"
"thread:                \n"
    "mrs     r1, psp    \n"
    "mov     r4, #0xF0  \n"      
    "b       common     \n"
"isr:                   \n"
    "mrs     r1, msp    \n"
    "mov     r4, #0x0F  \n"    
"common:                \n"
    "add     r3, r1, #24\n"
    "ldr     r2, [r3]   \n"
    "sub     r2, #2     \n"
    "ldrb    r0, [r2]   \n"
    "and     r0, r4     \n"      
    "b       svc_c      \n"
);
//pendsv isr
__asm__(
".thumb_func            \n"
".global pendsv_isr     \n"
"pendsv_isr:            \n"
    "mrs r0,psp         \n"
    "stmdb r0!, {r4-r11}\n"
    "ldr r4,=curr       \n"
    "ldr r3,[r4]        \n"
    "str r0,[r3]        \n"
    "ldr r1,=next       \n" 
    "ldr r2,[r1]        \n"  
    "ldr r0,[r2]        \n"  
    "mov r3,#0          \n"
    "str r3,[r1]        \n"
    "str r2,[r4]        \n"
    "ldmia r0!,{r4-r11} \n"
    "str r0,[r2]        \n"
    "msr psp,r0         \n"
    "bx lr              \n"
);

//first task second stage init
__asm__(
".thumb_func                    \n"
".global __init_second_stage    \n"
"__init_second_stage:           \n"
    "ldr r1,=__estack           \n"
    "msr msp,r1                 \n"
    "ldr r3,=curr               \n"
    "ldr r2,[r3]                \n"
    "ldr r0,[r2]                \n"
    "ldmia r0!,{r4-r11}         \n"
    "str r0,[r2]                \n"
    "msr psp, r0                \n"
    "mov lr, #0xFFFFFFFD        \n"
    "bx lr                      \n"
);
//helpers for specific common operations

__asm__(
".thumb_func        \n"
".global __ldrex    \n"
"__ldrex:           \n"
    "ldrex r0,[r0]  \n"
    "bx lr          \n"
);

__asm__(
".thumb_func            \n"
".global __strex        \n"
"__strex:               \n"
    "strex r0,r0,[r1]   \n"
    "bx lr              \n"
);
__asm__(
".thumb_func            \n"
".global __clrex        \n"
"__clrex:               \n"
    "clrex              \n"
    "bx lr              \n"
);

__asm__(
".thumb_func                \n"
".global __modify_basepri   \n"
"__modify_basepri:          \n"
    "mov r1,r0              \n" 
    "mrs r0, basepri        \n"
    "msr basepri,r1         \n"
    "isb                    \n"
    "bx lr                  \n"
);

__asm__(
".thumb_func                \n"
".global __restore_basepri  \n"
"__restore_basepri:         \n"
    "msr basepri,r0         \n"
    "isb                    \n"
    "bx lr                  \n"
);

__asm__(
".thumb_func            \n"
".global __set_control  \n"
"__set_control:         \n"
    "msr control, r0    \n"
    "bx lr              \n"
);
__asm__(
".thumb_func            \n"
".global __get_control  \n"
"__get_control:         \n"
    "mrs r0, control    \n"
    "bx lr              \n"
);
/**
 * \b svc_c
 *
 * Internal function, part of svc handler.
 * Dispatches svc codes to appropriate handlers
 *
 * This function should not be called by the application
 *
 */

void svc_c(uint8_t svc, uint32_t* stack){
    switch (svc) {
        //start first task
        case 0x4:{
            kernel_cb.is_running = 1;
            __set_control(0x1);
            __restore_basepri(0);
            curr = __dequeue_head(&kernel_cb.readyq,BAD_RTOS_MISC_READYQ_HEAD);
#ifdef BAD_RTOS_USE_MPU
            __task_mpu_apply_perms(curr);
#endif 
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
           stack[0]=__task_unblock((bad_tcb_t*) stack[0]);
           break;
        }
        case 0x3:
        case 0x30:{
            stack[0] =__task_delay_cancel((bad_tcb_t*)stack[0]);
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
        default:{
            stack[0] = BAD_RTOS_STATUS_WRONG_CONTEXT;
        }
    }
}



void systick_usr(){
    CRITICAL_SECTION_STORE
    if(!kernel_cb.is_running){
        return;
    }
    CRITICAL_SECTION;
    ticks++;
    BAD_SYSTICK_STATUS status = 0;
    
    if(next){//at this point pendsv is pending and curr is enqueued into a queue, 
             //updating its counter will shorten its delay by 2 ticks if its delayed or shorten its timeframe when its
             //no longer running 
        goto delayq_update;
    }

    curr->counter--;
    
    if(!curr->counter){
        status |= BAD_SYSTICK_TIMEFRAME_PENDING;
    }

delayq_update:
    if(kernel_cb.delayq){
        kernel_cb.delayq->counter--;
        if(!kernel_cb.delayq->counter){
            status|= BAD_SYSTICK_DELAY_WAKE_PENDING;
        }

    }
    if(status){    
        __sched_update_systick(status);
    }
    CRITICAL_SECTION_END;
}



ALWAYS_INLINE void __bad_rtos_interrupt_setup(){
    SCB_set_priority_grouping(SCB_PRIO_GROUP4);
    SCB_set_core_interrupt_priority(SCB_SVC_INTR, SCB_PRIO1);
    SCB_set_core_interrupt_priority(SCB_SYSTICK_INTR, SCB_PRIO15);
    SCB_set_core_interrupt_priority(SCB_PENDSV_INTR, SCB_PRIO15);
}

ALWAYS_INLINE void __bad_rtos_systick_setup(){
    systick_setup(CLOCK_SPEED/1000, SYSTICK_FEATURE_CLOCK_SOURCE|SYSTICK_FEATURE_TICK_INTERRUPT);
    systick_enable();
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

void bad_rtos_start(){

    __restore_basepri(2<<4);    
    __tcb_queue_slab_init();
#if defined (BAD_RTOS_USE_KHEAP)
    __buddy_init(&kernel_buddy, kheap, kfreelist, KMIN_ORDER, KMAX_ORDER);
#endif
#if defined (BAD_RTOS_USE_UHEAP) 
    __buddy_init(&user_buddy, uheap, ufreelist, UMIN_ORDER, UMAX_ORDER);
#endif 
#ifdef BAD_RTOS_USE_MPU
    __bad_rtos_mpu_default_setup();
#endif
    __bad_rtos_interrupt_setup();
    __bad_rtos_systick_setup();
    bad_task_descr_t idle_task_descr = {
        .stack = idle_stack,
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
