/**
// ///DOC_REPLACE_START
   * @file badrtos_armv7.h
* @brief Header only rtos implementation
*
* Usage:
*  - Include this file and define BAD_RTOS_IMPLEMENTATION in one
*    C file
*  - Change the config to your liking
*  - Define the bad_user_init function and all the perliminary setup there, like task creation 
*  - Call bad_rtos_start to start rtos operation
* Notes:
*  - Depends on the linker file , to port just edit the linker file adding nessesary sections at the beginning of ram :
*   .kernel_bss (NOLOAD) : ALIGN(32)
*   {
*       __kernel_bss = .;
*       *(.kernel_bss)
*       __ekernel_bss = .;
*   } > RAM
*
*   __rkernel_data = LOADADDR(.kernel_data);
*
*   .static_stacks : ALIGN(4096)
*   {
*       __static_stacks = .;
*       *(.static_stacks)
*       __estatic_stacks = .;
*   }
*   .heap : ALIGN(32)
*   {
*     __heap = .;
*     *(.kheap)
*   } > RAM
*
*  - ! Kernel syscall interrupt priority is 0 on startup ,
*      after startup it drops to lowest alowing isrs to run freely, 
*      all the interaction between the kernel and isrs are done through pendsv triggering functions
*      
*  - !! If the task uses FPU make sure the stack size can accomodate additional 33 registers

 // ///DOC_REPLACE_END

// PUBLIC API**********************************************
 
**
* \b BAD_TASK_HANDLE_IS_VALID
*  Public macro to check the validity of the task handle
*  
*  @param[in] bad_task_handle_t task handle
*
*  @retval 1 valid
*  @retval 0 invalid
*
#define BAD_TASK_HANDLE_IS_VALID(handle)
 
**
* \b BAD_TASK_HANDLE_GET_ERROR(handle)
*  Public macro to get error from taskhandle. If the handle is valid, 
*  evaluates to BAD_RTOS_STATUS_OK; otherwise evaluates to the error code 
*  stored in handle.gen.
*  
*  @param[in] bad_task_handle_t task handle
*  
*  @retval BAD_RTOS_STATUS_OK task handle valid
*  @retval BAD_RTOS_STATUS_BAD_PARAMETERS on bad configurations
*  @retval BAD_RTOS_STATUS_ALLOC_FAIL on allocation falure
*  @retval BAD_RTOS_STATUS_NOT_OWNER message queue passed already has an owner
* 
#define BAD_TASK_HANDLE_GET_ERROR(handle)
 
**
* \b bad_rtos_start
*
* Public function. Kernel entry point; call this once from main() after all 
* static tasks/objects are set up.
*
* Initialises kernel internal state (section/slab allocators, interrupt 
* vector handling, idle task, kernel heap if BAD_RTOS_USE_KHEAP, MPU default 
* regions if BAD_RTOS_USE_MPU, FPU access/settings if BAD_RTOS_USE_FPU), then 
* calls the user-supplied bad_user_init() (see below). If bad_user_init() 
* returns BAD_RTOS_STATUS_OK the scheduler starts the first ready task and 
* this function does not return; otherwise basepri is restored and 
* bad_rtos_start() returns to the caller.
*
* This function cannot be called from interrupt context.
*
* extern void bad_rtos_start();
 
**
* \b bad_user_init
*
* Function the application MUST provide (it is only declared extern here, 
* not implemented by this header). Called once by bad_rtos_start() after 
* kernel internals are initialised but before the scheduler starts. Use it 
* to create your initial tasks with task_make() and set up any static 
* synchronisation objects.
*
* @retval BAD_RTOS_STATUS_OK kernel proceeds to start the scheduler
* @retval any other value bad_rtos_start() aborts startup and returns instead
*
* extern bad_rtos_status_t bad_user_init();
 
**
* \b TASK_STATIC_STACK(task_name,size)
*
* Public macro. Declares a statically allocated task stack buffer, placed in 
* the ".static_stacks" linker section:
*   u8 task_name##_stack[size];
*
* size must be a multiple of 32 and at least 128 bytes when BAD_RTOS_USE_MPU 
* is enabled (region alignment/size requirements); a multiple of 8 and at 
* least 64 bytes otherwise. Both are enforced with _Static_assert. Point 
* bad_task_descr_t.stack at task_name##_stack and set .stack_size = size to 
* use it, instead of leaving .stack = 0 for a dynamically (kernel-heap) 
* allocated stack.
*
* #define TASK_STATIC_STACK(task_name,size)
 
**
* \b task_make
*
* Public SVC (svc 0xF4) call that calls internal function __task_make
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
* @retval bad_task_handle_t Task handle
* @retval invalid bad_task_handle_t on falure 
*
* extern bad_task_handle_t task_make(const bad_task_descr_t *descr);
 
**
* \b task_delay
*
* Public SVC (svc 0x7) call that calls internal function __task_delay
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
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @param[in] u32 delay in ticks 
* @param[in] cbptr cb callback to run 
* @param[in] void* args arguments for the callback
*
* @retval BAD_RTOS_STATUS_OK delay time ran out
* @retval BAD_RTOS_STATUS_WOKEN the task was woken by another task or isr
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t task_delay(u32 delay, cbptr cb, void *args );
 
**
* \b task_block
*
* Public SVC (svc 0x6) call that calls internal function __task_block
* Blocks the current task until another task or isr unblocks it 
*   
* Enqueues current task into an unordeded kernel list of blocked tasks  
* Then switches context to the highest priority ready task
* 
* Tasks are unblocked using task_unblock() public function
*
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @retval BAD_RTOS_STATUS_WOKEN task is successfully blocked
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr 
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t task_block();
 
**
* \b task_unblock
*
* Public SVC (svc 0x26) call that calls internal function __task_unblock
* Unblocks the specifed task and tries to preempt the current one
*
* Dequeues the specified task from unordeded kernel list of blocked tasks 
* If the task is not in blocked list(depending on the misc field) returns BAD_RTOS_STATUS_NOT_BLOCKED
*
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @param[in] bad_task_handle_t Task handle
*
* @retval BAD_RTOS_STATUS_OK task is successfully unblocked
* @retval BAD_RTOS_STATUS_NOT_BLOCKED the task is not blocked
* @retval BAD_RTOS_STATUS_HANDLE_INVALID handle is invalid
*
* extern bad_rtos_status_t task_unblock(bad_task_handle_t task);
 
**
* \b task_unblock_from isr
*
* Public kernel notification function that calls internal function __task_unblock
* Unblocks the specifed task and tries to preempt the current one
*
* Dequeues the specified task from unordeded kernel list of blocked tasks 
* If the task is not in blocked list(depending on the misc field)
*
* This function can be called from interrupt context.
* @param[in] bad_task_handle_t Task handle
*
* @retval BAD_RTOS_STATUS_OK task is successfully unblocked
* @retval BAD_RTOS_STATUS_HANDLE_INVALID handle is invalid
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT if called from thread context
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message 
*
* extern bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t task);
 
**
* \b task_yield
*
* Public SVC (svc 0x5) call that calls internal function __task_yield
* Tries to yield to a same priority task
*
* 
* If succedes enqueues current task into ready queue and yields to the task of the same priority if availible
* Then switches context 
*
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @retval BAD_RTOS_STATUS_OK task successfully yielded
* @retval BAD_RTOS_STATUS_CANT_YIELD no task to yield to
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t task_yield();
 
**
* \b task_finish
*
* Public SVC (svc 0x4) call that calls internal function __task_finish
* Finishes the execution of the task, frees the tcb and the stack if it was dynamically allocated
* 
* Call this only when every resourse held by task is released
*
* If task holds mutexes which is reflected in tcb->mutex_count tries to trap
*
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @retval BAD_RTOS_STATUS_CANT_FINISH task still holds mutexes, do not rely on this behavior, this is for debug only
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr 
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t task_finish();
 
**
* \b task_delay_cancel
*
* Public SVC (svc 0x27) call that calls internal function __task_delay_cancel
* Wakes the task from delay without running the callback
*
* Dequeues the specified task from kernel delay delta list
* Tries to preempt the currently running task 
*
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @param[in] bad_task_handle_t Task handle
*
* @retval BAD_RTOS_STATUS_OK tasks delay successfully canceled
* @retval BAD_RTOS_STATUS_NOT_DELAYED task is not delayed 
* @retval BAD_RTOS_STATUS_HANDLE_INVALID handle invalid
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t task_delay_cancel(bad_task_handle_t task);
 
**
* \b task_delay_cancel_from_isr
*
* Public kernel notification function that calls internal function __task_delay_cancel
* Wakes the task from delay without running the callback
*
* Dequeues the specified task from kernel delay delta list
* Tries to preempt the currently running task 
*
* This function can be called from interrupt context.
* @param[in] bad_task_handle_t Task handle
*
* @retval BAD_RTOS_STATUS_OK tasks delay successfully canceled
* @retval BAD_RTOS_STATUS_HANDLE_INVALID handle invalid 
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT if called from thread context
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message 
*
* extern bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t task);
 
// IRQ ownership api
**
* \b irq_acquire
*
* Public SVC (svc 0x19) call that calls internal function __irq_acquire
* Claims exclusive ownership of an interrupt/exception for the calling task.
* Every other irq_* call below only works on an irq the caller owns.
*
* irqn is the CMSIS style IRQn number: negative for core exceptions, 
* 0..(BAD_RTOS_IRQ_COUNT - 17) for peripheral interrupts. 
*
* An irq can be owned by one task at a time, and one task can own at most 3 
* irqs. SVC, PendSV and SysTick are used by the kernel and cannot be acquired.
*
* Ownership is only a kernel side bookkeeping. Acquiring does not touch the 
* hardware: it does not enable, clear or change the priority of the irq.
*
* Owned irqs are released automatically when the owner calls task_finish. 
* Their hardware state (enable, priority, pending) is left as it was, so 
* disable them first if that matters.
*
* This function is intended to be called from task context; ownership is 
* tracked against the current task.
*
* @param[in] s32 irqn irq number
*
* @retval BAD_RTOS_STATUS_OK irq acquired
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS irqn out of range, or one of SVC/PendSV/SysTick
* @retval BAD_RTOS_STATUS_ALLOC_FAIL caller already owns 3 irqs, or the irq is owned by another task
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_acquire(s32 irqn);
 
**
* \b irq_enable
*
* Public SVC (svc 0x20) call that calls internal function __irq_enable
* Enables the irq in the NVIC. The caller must own the irq.
*
* Core exceptions (irqn < 0): only a subset is implemented, the rest return 
* BAD_RTOS_STATUS_BAD_PARAMETERS. This applies to all irq_* calls below.
*
* @param[in] s32 irqn irq number
*
* @retval BAD_RTOS_STATUS_OK irq enabled
* @retval BAD_RTOS_STATUS_NOT_OWNER caller does not own the irq
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS core exception that is not supported
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_enable(s32 irqn);
 
**
* \b irq_disable
*
* Public SVC (svc 0x21) call that calls internal function __irq_disable
* Disables the irq in the NVIC. The caller must own the irq.
*
* @param[in] s32 irqn irq number
*
* @retval BAD_RTOS_STATUS_OK irq disabled
* @retval BAD_RTOS_STATUS_NOT_OWNER caller does not own the irq
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS core exception that is not supported
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_disable(s32 irqn);
 
**
* \b irq_pend
*
* Public SVC (svc 0x22) call that calls internal function __irq_pend
* Sets the irq pending. The caller must own the irq.
*
* @param[in] s32 irqn irq number
*
* @retval BAD_RTOS_STATUS_OK irq set pending
* @retval BAD_RTOS_STATUS_NOT_OWNER caller does not own the irq
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS core exception that is not supported
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_pend(s32 irqn);
 
**
* \b irq_clear
*
* Public SVC (svc 0x23) call that calls internal function __irq_clear
* Clears the pending state of the irq. The caller must own the irq.
*
* @param[in] s32 irqn irq number
*
* @retval BAD_RTOS_STATUS_OK pending state cleared
* @retval BAD_RTOS_STATUS_NOT_OWNER caller does not own the irq
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS core exception that is not supported
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_clear(s32 irqn);
 
**
* \b irq_set_prio
*
* Public SVC (svc 0x24) call that calls internal function __irq_set_prio
* Sets the priority of the irq. The caller must own the irq.
*
* prio must fit in BAD_RTOS_PRIO_BITS bits (0..15 with the default of 4). 
* Lower value means higher priority, as usual on Cortex-M.
*
* @param[in] s32 irqn irq number
* @param[in] u8 prio priority
*
* @retval BAD_RTOS_STATUS_OK priority set
* @retval BAD_RTOS_STATUS_NOT_OWNER caller does not own the irq
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS prio out of range, or core exception that is not supported
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_set_prio(s32 irqn, u8 prio);
 
**
* \b irq_release
*
* Public SVC (svc 0x25) call that calls internal function __irq_release
* Gives up ownership of the irq so another task can acquire it. 
* Like irq_acquire this does not touch the hardware state of the irq.
*
* @param[in] s32 irqn irq number
*
* @retval BAD_RTOS_STATUS_OK irq released
* @retval BAD_RTOS_STATUS_NOT_OWNER caller does not own the irq
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS invalid irqn
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t irq_release(s32 irqn);
 
**
* \b in_isr / \b in_task
*
* Public static inline helpers. Read the ipsr core register to determine 
* the current execution context.
*
* bad_context_in_isr()  returns non-zero when ipsr != 0 (running in an exception/ISR handler)
* bad_context_in_task() returns non-zero when ipsr == 0 (running in normal thread/task context)
*
* Either can be called from any context.
*
* static inline u32 bad_context_in_isr();
* static inline u32 bad_context_in_task();
 
**
* \b preempt_disable
*
* Public function. Increments a global nesting counter (preempt_count) that 
* suppresses rescheduling. Calls nest: each preempt_disable() must be matched 
* by a preempt_enable().
*
* Unlike most of the other synchronisation primitives in this file this is a 
* plain function call, not an SVC.
*
* This function cannot be called from interrupt context.
*
* extern void preempt_disable();
 
**
* \b preempt_enable
*
* Public function. Decrements the preempt_count nesting counter. When the 
* counter reaches 0 it triggers a reschedule check (svc 0xF0, internal 
* function __svc_check_resched) so a pending context switch can run.
*
* Calling this without a matching prior preempt_disable() (counter already 0) 
* traps.
*
* This function cannot be called from interrupt context.
*
* extern void preempt_enable();
 
**
* \b pool_init
*
* Public function 
* Initialises a pool allocator object over a user-supplied block of memory, 
* splitting it into fixed-size blocks of block_size bytes (size_in_bytes 
* must be an exact multiple of block_size).
*
* This function can be called from interrupt context. This function is NOT 
* reentrant if the object parameter is the same
* @param[in] bad_pool_t* pool object to initialise
* @param[in] void* mem backing memory block to carve into fixed-size blocks
* @param[in] u32 block_size size in bytes of each block
* @param[in] u32 size_in_bytes total size of mem, must be a multiple of block_size
*
* @retval BAD_RTOS_STATUS_OK pool successfully initialised
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS pool/mem is NULL, block_size or size_in_bytes is 0, or size_in_bytes is not a multiple of block_size
*
* extern bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, u32 block_size, u32 size_in_bytes);
 
**
* \b POOL_DEFINE(name,mem,block_size,size_in_bytes)
*
* Public macro. Declares and field-initialises a bad_pool_t object without 
* calling pool_init() at runtime:
*   bad_pool_t name = {.mem = mem, .block_size = block_size, .size_in_bytes = size_in_bytes}
*
* #define POOL_DEFINE(name,mem,block_size,size_in_bytes)
 
**
* \b pool_alloc
*
* Public function 
* Tries to allocate an object from specifed pool allocator
* If a freed block exsists pulls it from the freelist, otherwise lazily allocates it 
* from an assosiated block of memory
*
* This function can be called from interrupt context. This function is reentrant 
* @param[in] bad_pool_t pool to allocate from 
* 
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
*
* extern void* pool_alloc(bad_pool_t *pool);
 
**
* \b pool_free
*
* Public function 
* Tries to free an object from specifed pool allocator
* If a block is a part of provided pool allocators memory puts the object into pools free list,
* otherwise traps
*
* This function can be called from interrupt context. This function is reentrant
*
* @param[in] bad_pool_t pool to free to 
* 
* extern void pool_free(bad_pool_t *pool, void *obj);
 
**
* \b gpool_alloc
*
* Public function 
* Specialised pool_alloc function that operates on kernel provided global pool which
* can be used to allocate all synchro objects,(or any object 16 bytes in size)
* !!!EXCEPT message queues and pools
*
* This function can be called from interrupt context. This function is reentrant 
* 
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
*
* extern void* gpool_alloc();
 
**
* \b gpool_free
* 
* This function can be called from interrupt context. This function is reentrant
*
* extern void gpool_free(void *obj);
 
**
* \b kernel_alloc
*
* Public SVC (svc 0xF2) call that calls internal function __kernel_alloc
* Tries to allocate a specifed number of bytes from kernel heap
*
* Uses buddy allocator under the hood
*
* This function cannot be called from interrupt context. 
* @param[in] u32 size in bytes 
* 
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
*
* extern void* kernel_alloc(u32 size);
 
**
* \b kernel_free
*
* Public SVC (svc 0xF3) call that calls internal function __kernel_free
* Tries to free a specifed number of bytes allocated from kernel heap
*
* Uses buddy allocator under the hood
*
* This function cannot be called from interrupt context.
* @param[in] void * to allocated memory 
* @param[in] u32 size in bytes 
* 
*
* extern void kernel_free(void *block,u32 size);
 
// Priority inheriting mutex api
**
* \b mutex_init
*
* Public function to initialise mutex object
* Initialises linked list head and zero initialises other fields
*
* This function can be called from interrupt context. But is not reentrant if the object parameter is the same
* @param[in] bad_mutex_t* Ptr to mutex object to initialise
*
* @retval BAD_RTOS_STATUS_OK mutex successfully initialised
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
*
* extern bad_rtos_status_t mutex_init(bad_mutex_t *mut);
 
**
* \b MUTEX_DEFINE(name)
*
* Public macro. Declares and initialises a bad_mutex_t object without calling 
* mutex_init() at runtime:
*   bad_mutex_t name = {.blockedq = DLIST_INITIALISER(name.blockedq)}
*
* #define MUTEX_DEFINE(name)
 
**
* \b mutex_take
*
* Public SVC (svc 0xE) call that calls internal function __mutex_take
* Tries to take the mutex
* If the mutex has no owner then the caller becomes the mutexes owner, increasing his mutex count by 1  
* If it has an owner the behavior depends on the delay value specified
* If the owner == caller increments the rec_takes counter
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
* This api can be called recursively
*
* This function cannot be called from interrupt context.Will generate a fault if done so
*
* @param[in] bad_mutex_t* Ptr to mutex object to try take  
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Mutex successfully taken
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
* @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT function was called from an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero and blocking delay value was specified
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,u32 delay);
 
**
* \b mutex_put
*
* Public SVC (svc 0xD) call that calls internal function __mutex_put
* Tries to put the mutex
*
* If the rec_takes counter is non zero (mutex was taken recursively) 
* decremets the rec_takes counter and returns, else: 
* If the caller is the owner then the highest priority blocked task is woken with BAD_RTOS_STATUS_OK written to its 
* stacked registers, its callback is canceled and tries to preempt the current running task. 
* If there is no blocked task mutex becomes free. Previous owners mutex count is decreased
* by 1 and if it is 0 previous owners priority is reset to base priority
* 
* If the caller is not the owner BAD_RTOS_STATUS_NOT_OWNER returned
*
*
* This function cannot be called from interrupt context.Will generate a fault if done so
*
* @param[in] bad_mutex_t* Ptr to mutex object to try put  
*
* @retval BAD_RTOS_STATUS_OK Mutex successfully put
* @retval BAD_RTOS_STATUS_NOT_OWNER caller is not the owner of this mutex object
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex object is NULL
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
* extern bad_rtos_status_t mutex_put(bad_mutex_t *mut);
 
**
* \b mutex_delete
*
* Public SVC (svc 0xF) call that calls internal function __mutex_delete
* Tries to delete the mutex object, doesnt infuence the underlying memory, just resets the object
*
* If the caller is the owner then wakes up all the tasks with BAD_RTOS_STATUS_DELETED written into their 
* stacked registers 
* If the caller is not the owner BAD_RTOS_STATUS_NOT_OWNER returned
*
*
* This function cannot be called from interrupt context.Will generate a fault if done so
* @param[in] bad_mutex_t* Ptr to mutex object to try delete  
*
* @retval BAD_RTOS_STATUS_OK Mutex successfully deleted
* @retval BAD_RTOS_STATUS_NOT_OWNER caller is not the owner of this mutex object
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex object is NULL
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t mutex_delete(bad_mutex_t *mut);
 
// Blocking semaphore api 
**
* \b sem_init
*
* Public function to initialise semaphore object
* initialises count field to the specifed count
*
* This function can be called from interrupt context. But is not reentrant if the object parameter is the same
* @param[in] bad_sem_t* Ptr to semaphore object to initialise
* @param[in] u32 Value to initialise semaphore counter with
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore ptr is null 
*
* extern bad_rtos_status_t sem_init(bad_sem_t *sem,u32 reset_value);
 
**
* \b SEM_DEFINE(name,count)
*
* Public macro. Declares and initialises a bad_sem_t object without calling 
* sem_init() at runtime:
*   bad_sem_t name = {.init_flag = 1, .counter = (count), .blockedq = DLIST_INITIALISER(name.blockedq)}
*
* #define SEM_DEFINE(name,count)
 
**
* \b sem_take
*
* Public SVC (svc 0xB) call that calls internal function __sem_take
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
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @param[in] bad_sem_t* Ptr to semaphore object to try take  
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Semaphore successfully taken
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero and blocking delay value was specified
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
* extern bad_rtos_status_t sem_take(bad_sem_t *sem,u32 delay);
 
**
* \b sem_put
*
* Public SVC (svc 0xA) call that calls internal function __sem_put
* Tries to put the semaphore
*
* If the semaphores counter is 0 and a blocked task exists the highest priority blocked task 
* is woken with BAD_RTOS_STATUS_OK written to its 
* stacked registers, its callback is canceled and tries to preempt the current running task. 
* If there is no blocked task semaphore counter is incremented. 
*
*
* This function cannot be called from interrupt context. Will generate a fault if done so
* @param[in] bad_sem_t* Ptr to sem object to try put  
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully put
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
* extern bad_rtos_status_t sem_put(bad_sem_t *sem);
 
**
* \b sem_put_from_isr
*
* Public kernel notification function 
* Tries to put the semaphore from isr
*
* If the semaphores counter is 0 and a blocked task exists the highest priority blocked task 
* is woken with BAD_RTOS_STATUS_OK written to its 
* stacked registers, its callback is canceled and tries to preempt the current running task. 
* If there is no blocked task semaphore counter is incremented. 
*
*
* This function must be called from interrupt context 
* @param[in] bad_sem_t* Ptr to sem object to try put  
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully put
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT if called from thread context
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message object
*
* extern bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem);
 
**
* \b sem_delete
*
* Public SVC (svc 0xC) call that calls internal function __sem_delete
* Tries to delete the semaphore object, doesnt infuence the underlying memory, just resets the object
*
* Wakes up all the tasks with BAD_RTOS_STATUS_DELETED written into their 
* stacked registers 
*
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @param[in] bad_sem_t* Ptr to semaphore object to try delete  
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully deleted
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t sem_delete(bad_sem_t *sem);
 
//Message queues
**
* \b MSGQ_DEFINE(name,size)
*
* Public macro. Declares a message queue object plus its backing message 
* array as ordinary (non-static) file/block-scope variables:
*   bad_msg_block_t name##_blocks[size];
*   bad_msgq_t name = { .capacity_mask = size - 1, .msgs = name##_blocks, ... };
*
* size MUST be a power of 2 (enforced with a _Static_assert). This only 
* declares and zero/field-initialises the storage; the declaring task still 
* needs to call msgq_acquire() to bind itself as owner before pulling messages.
*
* #define MSGQ_DEFINE(name,size)
 
**
* \b MSGQ_DEFINE_STATIC(name,size)
*
* Public macro. Same as MSGQ_DEFINE(), but declares both the backing array 
* and the bad_msgq_t object with the `static` storage class, so they are 
* private to the translation unit they're declared in.
*
* #define MSGQ_DEFINE_STATIC(name,size)
 
//Heap dependant api
**
* \b msgq_acquire_allocate
*
* Public SVC call (svc 0x14) that calls internal function __msgq_acquire_allocate.
* Dynamically binds a message queue to the currently running task and allocates kernel memory for its buffer.
*
* The current task becomes the exclusive owner of this message queue (receivers must be owners).
* The capacity must be a power of 2.
*
* @param[in] bad_msgq_t* q Ptr to message queue object to initialize and bind
* @param[in] u16 capacity Number of messages the queue can hold (MUST be a power of 2)
*
* @retval BAD_RTOS_STATUS_OK Queue successfully allocated and bound to current task
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL or capacity is not a power of 2
* @retval BAD_RTOS_STATUS_NOT_OWNER Queue is already owned by another task
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t msgq_acquire_allocate(bad_msgq_t *q, u16 capacity);
 
**
* \b msgq_release_deallocate
*
* Public SVC call (svc 0x15) that calls internal function __msgq_release_deallocate.
* Unbinds the message queue from the current task and frees the dynamically allocated kernel memory.
*
* Wakes up all tasks currently blocked (waiting to post to this queue) with BAD_RTOS_STATUS_DELETED
* written into their stacked registers. Resets the message queue object to 0.
*
* @param[in] bad_msgq_t* q Ptr to dynamically allocated message queue object to release
*
* @retval BAD_RTOS_STATUS_OK Queue successfully deallocated and unbound
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL or queue was not dynamically allocated
* @retval BAD_RTOS_STATUS_NOT_OWNER Current task is not the owner of this queue
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t msgq_release_deallocate(bad_msgq_t *q);
 
//Heap independant api
**
* \b msgq_acquire
*
* Public SVC call (svc 0x12) that calls internal function __msgq_acquire.
* Statically binds a message queue to the currently running task without allocating memory.
*
* Assumes the message queue buffer has already been provisioned.
* The current task becomes the exclusive owner of this message queue.
* @param[in] bad_msgq_t* q Ptr to static message queue object to bind
*
* @retval BAD_RTOS_STATUS_OK Queue successfully bound to current task
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_OWNER Queue is already owned by another task
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t msgq_acquire(bad_msgq_t *q);
 
**
* \b msgq_release
*
* Public SVC call (svc 0x13) that calls internal function __msgq_release.
* Unbinds a statically provisioned message queue from the current task.
*
* Resets the queue's head pointers and wakes up all tasks currently blocked 
* (waiting to post) with BAD_RTOS_STATUS_DELETED written into their stacked registers.
*
* @param[in] bad_msgq_t* q Ptr to static message queue object to release
*
* @retval BAD_RTOS_STATUS_OK Queue successfully unbound
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_OWNER Current task is not the owner of this queue
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t msgq_release(bad_msgq_t *q);
 
**
* \b msgq_pull_msg
*
* Public SVC call (svc 0x11) that calls internal function __msgq_pull_msg.
* Tries to pull (receive) a message from the queue. Only the owner task can pull messages.
*
* If the queue is empty, the behavior depends on the delay value specified:
* delay = 0 : task is blocked until a message arrives.
* delay = -1 : pull fails and returns immediately.
* delay = N : task tries to pull for N ticks. If N ticks pass, task is woken with timeout status.
*
* If space frees up in the queue after pulling, a blocked publisher task is awakened.
*
* @param[in] bad_msgq_t* q Ptr to message queue to pull from
* @param[out] bad_msg_block_t* writeback Ptr to memory where the pulled message will be copied
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Message successfully pulled
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q or writeback ptr is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_NOT_OWNER Current task is not the owner of this queue
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay is -1 and queue is empty
* @retval BAD_RTOS_STATUS_TIMEOUT blocked for N ticks without receiving a message
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback, u32 delay);
 
**
* \b msgq_post_msg
*
* Public SVC call (svc 0x10) that calls internal function __msgq_post_msg.
* Tries to post a message (signal + args) to the queue. Any task can post to the queue.
*
* If the queue is full, the behavior depends on the delay value specified:
* delay = 0 : task is blocked until space becomes available.
* delay = -1 : post fails and BAD_RTOS_STATUS_WOULD_BLOCK is returned.
* delay = N : task blocks for N ticks waiting for space.
*
* If the queue was previously empty and the owner is waiting, the owner is awakened 
* and receives the message immediately.
*
* @param[in] bad_msgq_t* q Ptr to message queue to post to
* @param[in] u32 signal The 32-bit signalID of the message
* @param[in] void* args Ptr to message arguments or payload
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Message successfully posted
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay is -1 and queue is full
* @retval BAD_RTOS_STATUS_TIMEOUT blocked for N ticks without space freeing up
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero and blocking delay value was specified
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t msgq_post_msg(bad_msgq_t *q, u32 signal, void *args, u32 delay);
 
**
* \b msgq_post_msg_from_isr
*
* Public kernel notification function.
* Tries to post a message to the queue from an ISR context.
*
* If the queue is full, the post fails and WOULD_BLOCK is returned (ISRs cannot block).
* If the queue was empty and the owner task was preempted while waiting, a PendSV 
* kernel notification is triggered to wake the consumer.
*
* This function must be called from an interrupt context.
*
* @param[in] bad_msgq_t* q Ptr to message queue to post to
* @param[in] u32 signal The 32-bit signal/ID of the message
* @param[in] void* args Ptr to message arguments or payload
*
* @retval BAD_RTOS_STATUS_OK Message successfully posted
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT Called from thread context instead of ISR
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK Queue is full, cannot post
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message
*
* extern bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, u32 signal, void *args);
 
//Event barrier 
**
* \b EVENT_BARRIER_FLAGS_VALID_MASK
*  Public mask (bit 31, 0x80000000UL). event_barrier_wait() and
*  event_barrier_fire()/_from_isr() OR this bit into the flags word on
*  success, so a caller can tell a valid flags/error return apart from a
*  bad_rtos_status_t error code, which never has bit 31 set.
*
#define EVENT_BARRIER_FLAGS_VALID_MASK (0x80000000UL)
 
**
* \b EVENT_BARRIER_GET_FLAGS(flags)
*  Public macro to extract the fired flag bits from a value returned by
*  event_barrier_wait(). Masks out EVENT_BARRIER_FLAGS_VALID_MASK.
*
*  @param[in] u32 flags value returned by event_barrier_wait()
*
*  @retval u32 the fired event flags, with the valid bit cleared
*  @retval 0 flags was not a valid (bit-31-set) return
*
#define EVENT_BARRIER_GET_FLAGS(flags)
 
**
* \b EVENT_BARRIER_GET_ERROR(flags)
*  Public macro to check whether a value returned by event_barrier_wait()
*  represents success or an error.
*
*  @param[in] u32 flags value returned by event_barrier_wait()
*
*  @retval BAD_RTOS_STATUS_OK flags is a valid (bit-31-set) return
*  @retval other the error code, transformed as (flags ^ EVENT_BARRIER_FLAGS_VALID_MASK)
*
#define EVENT_BARRIER_GET_ERROR(flags)
 
**
* \b event_barrier_prime
*
* Public function that primes an event barrier for a new synchronization cycle.
* Initializes the barrier to wait for a specific number of distinct event flags.
* * The count specifies how many distinct flags must be set before the barrier fires. 
* The maximum number of flags is 31.
*
* @param[in] bad_event_barrier_t* Ptr to event barrier object to prime
* @param[in] u32 count Number of distinct events required to fire the barrier (1 to 31)
*
* @retval BAD_RTOS_STATUS_OK Event barrier successfully primed
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier ptr is null, count is 0, or count >= 32
* @retval BAD_RTOS_STATUS_IN_USE barrier is currently active/primed and hasn't fired yet
*
* extern bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, u32 count);
 
**
* \b event_barrier_wait
*
* Public svc call (svc 0x16) that calls internal function __event_barrier_wait
* Blocks the current task until the event barrier fires (accumulates the required number of flags).
* * If the barrier has not yet fired, the task is inserted into the event barrier's blocking priority queue.
* The behavior depends on the delay value specified:
* 
* delay = 0 : task is blocked indefinitely.
* delay = N : task tries to wait for N ticks. If N ticks pass without the barrier firing, 
* the task is removed from the queue and awakened with a timeout status.
* delay = -1 : wait fails and BAD_RTOS_STATUS_WOULD_BLOCK is returned 
*
* When the event barrier fires all tasks are woken with flags at the time of firing with bit 31 set (1 << 31)
* 
* @param[in] bad_event_barrier_t* Ptr to event barrier object to wait on
* @param[in] u32 delay ticks 0 = block, N = block for N ticks
*
* @retval u32 flags | (1 << 31)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier ptr is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired (count == 32)
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay = -1 and barrier has not fired yet 
* @retval BAD_RTOS_STATUS_SCHED_LOCKED preempt counter not zero and blocking delay value was specified
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern u32 event_barrier_wait(bad_event_barrier_t *event_barrier, u32 delay);
 
**
* \b event_barrier_fire_from_isr
*
* Public kernel notification function.
* Sets a specific event flag on the barrier from an ISR context.
*
* Performs an atomic update of the barriers flags. If the addition of this flag 
* satisfies the barrier's required event count, the barrier fires (count is set to 32). 
* When fired, it generates a kernel notification to wake up all blocked tasks.
*
* This function must be called from an interrupt context.
*
* @param[in] bad_event_barrier_t* Ptr to event barrier object to fire
* @param[in] u32 flag Bitmap representing the specific event(s) to set
*
* @retval BAD_RTOS_STATUS_OK Flag successfully set (barrier may or may not have fired)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier is NULL, flag is 0, or flag contains invalid bits
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT if called from thread context instead of ISR
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message
*
* extern bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier, u32 flag);
 
**
* \b __event_barrier_fire
*
* Public svc call (svc 0x17) that calls internal function __event_barrier_fire
* Sets a specific event flag on the barrier from a thread context.
*
* Performs an atomic update of the barriers flags. If the addition of this flag 
* satisfies the barriers required event count, the barrier fires (count is set to 32).
* When fired, it immediately unblocks all tasks waiting on this barrier and writes the flags state to the stacks of blocked tasks
*
* @param[in] bad_event_barrier_t* Ptr to event barrier object to fire
* @param[in] u32 flag Bitmap representing the specific event(s) to set
*
* @retval BAD_RTOS_STATUS_OK Flag successfully set (barrier may or may not have fired)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier is NULL, flag is 0, or flag contains invalid bits
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t event_barrier_fire(bad_event_barrier_t *event_barrier, u32 flag);
 
**
* \b event_barrier_delete
*
* Public svc call (svc 0x18) that calls internal function __event_barrier_delete
* Resets the event barrier object. Does not free underlying memory, just clears state.
*
* Wakes up all tasks waiting in the barriers blocked queue with BAD_RTOS_STATUS_DELETED 
* written into their stacked return registers.
*
* @param[in] bad_event_barrier_t* Ptr to event barrier object to delete/reset
*
* @retval BAD_RTOS_STATUS_OK barrier successfully reset
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier object is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier is already unprimed/count is 0
* @retval BAD_RTOS_STATUS_NOT_RUNNING function called before rtos has started
*
* extern bad_rtos_status_t event_barrier_delete(bad_event_barrier_t *event_barrier);
 
// Mpu 
 ** NOTE : Per-task MPU regions must 
*  be built by hand as a plain, NULL/zero-terminated array of 
* bad_mpu_user_region_t entries ({addr, size, type, settings}) and assigned 
* to bad_task_descr_t.regions. There is no region_count field on 
* bad_task_descr_t and no MPU_REGIONS_SIZE macro: the array length is 
* determined by its terminating (zero-size) entry, not a count.
* 
* NOTE : ARMv7M mpu regions dont have Transitive/Nontransitive property
* so WT/WB opts map to the same translation
*
* This array is capped at 3 real region entries plus the terminating entry 
* (4 slots total) — there is intentionally no macro to raise this, so size 
* your regions accordingly.
*
* /b DEFINE_DMA_BUFF
* #define DEFINE_DMA_BUFF(name,size)
 
* Usage example:
*    static const bad_mpu_user_region_t task1_regions[] = {
*        { .addr = (u8*)USART1_BASE, .size = sizeof(USART_typedef_t),
*          .type = BAD_MPU_REGION_DEVICE_NGRE, .settings = 0 },
*        { 0 } //terminating entry
*    };
* Then in task creation:
*    bad_task_descr_t task1_descr = {
*      .stack = 0,
*      .stack_size = TASK1_STACK_SIZE,
*      .entry = task1,
*      .args = 0,
*      .regions = task1_regions,
*      .ticks_to_change = 500,
*      .base_priority = TASK2_PRIORITY
*  };
* (.stack = 0 with a non-zero .stack_size causes the stack to be dynamically 
* allocated from the kernel heap; supply a pointer in .stack to use a 
* statically provisioned stack instead.)
*

// Iter sections
**
* \b BAD_ITER_SECTION_MEMBER(section_name,type,var_name) / 
* \b BAD_ITER_SECTION_EXTERN(section_name,type) / 
* \b BAD_ITER_SECTION_ITER_ALL(section_name,type,pos)
*
* Public "linker set" trick: lets independent translation units each 
* contribute one or more const objects to a single, contiguous, iterable 
* table, without any of them knowing about each other or registering into a 
* central array by hand. Used for things like self-registering driver 
* descriptors or static registration tables.
*
* How it works:
* - BAD_ITER_SECTION_MEMBER(section_name,type,var_name) declares one const 
*   object of `type`, named `var_name`, and places it into a custom linker 
*   section literally named `section_name` 
* - BAD_ITER_SECTION_EXTERN(section_name,type) declares extern references to 
*   __start_##section_name and __stop_##section_name. These two symbols are 
*   auto-generated for free by the GNU linker for any section whose name is 
*   a valid C identifier, and they bracket the start and one-past-the-end of 
*   the merged section, i.e. of every BAD_ITER_SECTION_MEMBER contributed to 
*   it across the whole link.
* - BAD_ITER_SECTION_ITER_ALL(section_name,type,pos) expands to a for-loop 
*   header that walks `pos` as a `const type *` from &__start_##section_name 
*   up to &__stop_##section_name
*
* section_name MUST be a valid C identifier (letters, digits, underscores 
* only) in all three macros, since it is both pasted into __start_/__stop_ 
* symbol names and turned into a linker section name.
*
* The order entries appear in while iterating is link order, not something 
* this API lets you control or rely on.
*
* Usage example:
*    //driver_a.c
*    BAD_ITER_SECTION_MEMBER(driver_table, driver_descr_t, drv_a) = { ... };
*    //driver_b.c
*    BAD_ITER_SECTION_MEMBER(driver_table, driver_descr_t, drv_b) = { ... };
*    //somewhere that iterates them, e.g. bad_user_init()
*    BAD_ITER_SECTION_EXTERN(driver_table, driver_descr_t);
*    const driver_descr_t *pos;
*    BAD_ITER_SECTION_ITER_ALL(driver_table, driver_descr_t, pos)
*    {
*        pos->init();
*    }
*
* #define BAD_ITER_SECTION_MEMBER(section_name, type, var_name)
* #define BAD_ITER_SECTION_EXTERN(section_name,type)
* #define BAD_ITER_SECTION_ITER_ALL(section_name,type,pos)
*/

#pragma once
#ifndef BAD_RTOS_H
#define BAD_RTOS_H

#include <stdint.h>
#include <stdbool.h>

#define KB (1024)

//CONFIG
//uncoment those to enable desired functionality
#define BAD_RTOS_USE_KHEAP      //kernel heap
//# define KMIN_ORDER 5          //kernel heap minimal order of allocation (size = 1 << MIN_ORDER = 32)
//# define KMAX_ORDER 12         //kernel heap maximum order of allocation (heap_size) (size = 1 << MIN_ORDER = 4096)
#define BAD_RTOS_USE_EVENT_BARRIER
#define BAD_RTOS_USE_MUTEX      //mutexes
# define BAD_RTOS_MUTEX_MAX_PI_ITERS 8 //Maximum depth PI walk can reach, also affects deadlock prevention
//# define BAD_RTOS_MUTEX_SIMPLE_PI // Simple PI, doesnt propagate and only affects the position in the readyq
#define BAD_RTOS_USE_MSGQ       // message queues
#define BAD_RTOS_USE_SEMAPHORE  //semaphores
#define BAD_RTOS_USE_MPU        //mpu
#define BAD_RTOS_USE_FPU        //fpu
#define BAD_RTOS_FPU_DEFAULT_SETTINGS //use default settings for the fpu (lazy + auto stacking enabled),if custom settings used - comment this and enable lazy stacking

#define BAD_RTOS_FLASH_RO_ADDR (0x08000000) //start of RO region
#define BAD_RTOS_FLASH_RO_SIZE (512 * KB)//used to setup mpu RO region
#define BAD_RTOS_RAM_ADDR (0x20000000) //start of RAM 
#define BAD_RTOS_RAM_SIZE (128 * KB)//used to setup mpu RAM region
#define BAD_RTOS_IRQ_COUNT (16 + 131) // core + nvic irqs

#define BAD_RTOS_GLOBAL_POOL_SIZE   (128)
#define BAD_RTOS_MAX_TASKS          (32)   //maximum number of running tasks, number of user priorities = 0..BAD_RTOS_MAX_TASKS-2, with idle task running at BAD_RTOS_MAX_TASKS-1
#define BAD_RTOS_PRIO_BITS          (4)

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

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

// error codes 
typedef enum 
{
    BAD_RTOS_STATUS_OK,
    BAD_RTOS_STATUS_ALLOC_FAIL,
    BAD_RTOS_STATUS_BAD_PARAMETERS,
    BAD_RTOS_STATUS_NOT_BLOCKED,
    BAD_RTOS_STATUS_NOT_DELAYED,
    BAD_RTOS_STATUS_HANDLE_INVALID,
    BAD_RTOS_STATUS_WOULD_BLOCK,
    BAD_RTOS_STATUS_CANT_YIELD,
    BAD_RTOS_STATUS_CANT_FINISH,
    BAD_RTOS_STATUS_TIMEOUT,
    BAD_RTOS_STATUS_WRONG_Q,
    BAD_RTOS_STATUS_NOT_OWNER,
    BAD_RTOS_STATUS_WOKEN,
    BAD_RTOS_STATUS_DELETED,
    BAD_RTOS_STATUS_NOT_INITIALISED,
    BAD_RTOS_STATUS_WRONG_CONTEXT,
    BAD_RTOS_STATUS_SCHED_LOCKED,
    BAD_RTOS_STATUS_FIRED,
    BAD_RTOS_STATUS_IN_USE,
    BAD_RTOS_STATUS_DEADLOCK,
    BAD_RTOS_STATUS_NOT_RUNNING
} bad_rtos_status_t;

typedef enum 
{
    BAD_RTOS_MISC_RUNNING,
    BAD_RTOS_MISC_READYQ_MEMBER,
    BAD_RTOS_MISC_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER,
} bad_rtos_misc_t;

typedef enum
{
    BAD_RTOS_MISC_NOT_DELAYED,
    BAD_RTOS_MISC_DELAYQ_MEMBER,
    BAD_RTOS_MISC_DELAYQ_SYNCHRO,
} bad_rtos_delayq_misc_t;

#ifdef BAD_RTOS_USE_MPU
typedef enum
{
    BAD_MPU_PRIV_FAULT_UNPRIV_FAULT = 0,
    BAD_MPU_PRIV_RW_UNPRIV_FAULT    = 1,
    BAD_MPU_PRIV_RW_UNPRIV_RW       = 1 << 1,
    BAD_MPU_PRIV_RO_UNPRIV_FAULT    = 1 << 2,
    BAD_MPU_PRIV_RO_UNPRIV_RO       = 1 << 3,
#define BAD_MPU_PRIV_MASK ((BAD_MPU_PRIV_RO_UNPRIV_RO << 1) - 1)
    BAD_MPU_NON_SHAREABLE           = 0,
    BAD_MPU_OUTER_SHAREABLE         = 1 << 4,
    BAD_MPU_INNER_SHAREABLE         = 1 << 5,
#define BAD_MPU_SH_MASK (BAD_MPU_OUTER_SHAREABLE | BAD_MPU_INNER_SHAREABLE)
    BAD_MPU_EXECUTE_NEVER           = 1 << 7
} bad_mpu_settings_t;

typedef enum
{
    BAD_MPU_REGION_NONE,
    BAD_MPU_REGION_DEVICE_GRE,
    BAD_MPU_REGION_DEVICE_NGRE,
#define BAD_MPU_DEVICE_REGIONS_END (BAD_MPU_REGION_DEVICE_NGRE)
    BAD_MPU_REGION_NORMAL_NONCACHEABLE,
#define BAD_MPU_REGION_DMA_BUFF (BAD_MPU_REGION_NORMAL_NONCACHEABLE)
    BAD_MPU_REGION_NORMAL_NT_CACHEABLE_WB,
    BAD_MPU_REGION_NORMAL_NT_CACHEABLE_WT,
    BAD_MPU_REGION_NORMAL_TR_CACHEABLE_WB,
    BAD_MPU_REGION_NORMAL_TR_CACHEABLE_WT,
    BAD_MPU_REGION_MAX,
} bad_mpu_region_type_t;

typedef struct 
{
    u8 *addr;
    u32 size;
    bad_mpu_region_type_t type;
    u32 settings;
} bad_mpu_user_region_t;

//mpu region struct, internal
typedef struct 
{
    u32 __reg0;
    u32 __reg1;
} bad_mpu_region_t;
#endif

#define DEFINE_DMA_BUFF(name,size) \
_Static_assert((size) % 32 == 0,"DMA buffers should be multiples of 32");\
static u8 __attribute__((section(".dma_buffs"))) name[(size)];

typedef union
{
    struct
    {
        u16 gen;
        u16 idx;
    };
    u32 val;
    
} bad_task_handle_t;

typedef void (*taskptr)(void * par) ;
typedef void (*cbptr)(bad_task_handle_t handle,void * par);

typedef struct bad_link_node bad_link_node_t;
struct bad_link_node
{
    bad_link_node_t *prev;
    bad_link_node_t *next;
};

typedef struct bad_llink_node bad_llink_node_t;
struct bad_llink_node
{
    bad_llink_node_t * volatile next;
};

// main fat struct of the program
typedef struct bad_tcb bad_tcb_t;
struct bad_tcb
{
    u32 *sp;
    //stack base
    u8 *stack;
    u32 stack_size;
    taskptr entry;
    //callback for delays
    cbptr cbptr;
    //args for the callback
    void* args;
    bad_link_node_t qnode;
    bad_link_node_t delaynode;
    u32 ticks_to_change;   
    volatile u32 counter;
#ifdef BAD_RTOS_USE_MPU
    bad_mpu_region_t regions[4];
#endif
    bad_link_node_t *blocked_on;
    bad_rtos_misc_t misc;
    bad_rtos_delayq_misc_t delayq_misc;
    u16 generation; //for handles
    s16 owned_irqs[3];
    u8 irqs_allocated;
#ifdef BAD_RTOS_USE_KHEAP
    u8 dyn_stack;
#endif
    //execution priority, follows nvic logic : lower number is higher priority
    u8 base_priority;
    u8 raised_priority;
#ifdef BAD_RTOS_USE_MUTEX
    u8 mutex_count;
#endif
#ifdef BAD_RTOS_USE_MSGQ
    u8 msgq_owner;
#endif
};

typedef struct 
{
    bad_llink_node_t freelist;
    u8 *mem;
    volatile u32 curr;
    u32 size_in_bytes;
    u32 block_size;
} bad_pool_t;

#ifdef BAD_RTOS_USE_MSGQ
typedef struct
{
    u32 signal;
    void *args;
} bad_msg_block_t;

typedef struct 
{
    bad_link_node_t blockedq;
    bad_tcb_t *owner;
    u16 capacity_mask;
    u16 pad;
    
    union
    {
        struct
        {
            volatile u16 head;
            volatile u16 tail;
        };
        
        volatile u32 atomic_update;
    };
    
    bad_msg_block_t *msgs;
    u8 dynamic;
} bad_msgq_t;
#endif

//task decription for task creation
typedef struct
{
    u32 stack_size;
    u8 *stack;
    taskptr entry;
    void *args;
    u32 ticks_to_change;
#ifdef BAD_RTOS_USE_MPU
    const bad_mpu_user_region_t *regions; //null terminated array
#endif
#ifdef BAD_RTOS_USE_MSGQ
    bad_msgq_t *assigned_msgq;
#endif
    u8 base_priority;
} bad_task_descr_t;

#ifdef BAD_RTOS_USE_MUTEX
typedef struct 
{
    bad_link_node_t blockedq;
    bad_tcb_t *owner;
    u32 rec_takes;
} bad_mutex_t;
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
typedef struct 
{
    bad_link_node_t blockedq;
    volatile u32 counter;
    volatile u32 init_flag;
} bad_sem_t;
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
typedef struct
{
    bad_link_node_t blockedq;
    volatile u32 flags;
    volatile u32 count;
} bad_event_barrier_t;
#endif

#ifdef BAD_RTOS_USE_MPU 
# define BAD_RTOS_STACK_ALIGN (32)
# ifdef BAD_RTOS_USE_FPU
#  define BAD_RTOS_MIN_STACK_SIZE (256)
# else
#  define BAD_RTOS_MIN_STACK_SIZE (128)
# endif 
#else
# define BAD_RTOS_STACK_ALIGN (8) 
# ifdef BAD_RTOS_USE_FPU
#  define BAD_RTOS_MIN_STACK_SIZE (208)
# else 
#  define BAD_RTOS_MIN_STACK_SIZE (72)
# endif 
#endif 

#define __STR_HELPER(x) #x
#define __STR(x) __STR_HELPER(x)

//Macro for static stack definition
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert(size % BAD_RTOS_STACK_ALIGN == 0,"Stack sizes must be multiples of " __STR(BAD_RTOS_STACK_ALIGN));\
_Static_assert(size >= BAD_RTOS_MIN_STACK_SIZE,"Stacks must be at least  " __STR(BAD_RTOS_MIN_STACK_SIZE) " bytes");\
u8 task_name##_stack[(size)] __attribute__((section(".static_stacks")));

//PUBLIC API**********************************************
//Main API
extern void bad_rtos_start();

#define BAD_TASK_HANDLE_IS_VALID(handle) ({ handle.idx != 0xFFFF; })

#define BAD_TASK_HANDLE_GET_ERROR(handle) ({\
BAD_TASK_HANDLE_IS_VALID(handle) ? BAD_RTOS_STATUS_OK : handle.gen; \
})

extern bad_task_handle_t task_make(const bad_task_descr_t *descr);
extern bad_rtos_status_t task_delay(u32 delay, cbptr cb, void *args );
extern bad_rtos_status_t task_block();
extern bad_rtos_status_t task_unblock(bad_task_handle_t task);
extern bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t task);
extern bad_rtos_status_t task_yield();
extern bad_rtos_status_t task_finish();
extern bad_rtos_status_t task_delay_cancel(bad_task_handle_t task);
extern bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t task);
extern bad_rtos_status_t irq_acquire(s32 irqn);
extern bad_rtos_status_t irq_enable(s32 irqn);
extern bad_rtos_status_t irq_disable(s32 irqn);
extern bad_rtos_status_t irq_pend(s32 irqn);
extern bad_rtos_status_t irq_clear(s32 irqn);
extern bad_rtos_status_t irq_set_prio(s32 irqn, u8 prio);
extern bad_rtos_status_t irq_release(s32 irqn);
extern void preempt_disable();
extern void preempt_enable();

#define POOL_DEFINE_TYPE(name,type,count) \
_Static_assert(sizeof(type) >= 4, "Pool elements should be at least 4 bytes");\
u8 __attribute__((aligned(_Alignof(type)))) name##_mem[sizeof(type) * (count)];\
bad_pool_t name = {.mem = name##_mem,.block_size = sizeof(type),.size_in_bytes = sizeof(type) * (count)};

#define POOL_DEFINE_TYPE_STATIC(name,type,count) \
_Static_assert(sizeof(type) >= 4, "Pool elements should be at least 4 bytes");\
static u8 __attribute__((aligned(_Alignof(type)))) name##_mem[sizeof(type) * (count)];\
static bad_pool_t name = {.mem = name##_mem,.block_size = sizeof(type),.size_in_bytes = sizeof(type) * (count)};

#define POOL_DEFINE(name,__mem,__block_size,__size_in_bytes)\
_Static_assert(sizeof(__block_size) >= 4, "Pool elements should be at least 4 bytes");\
bad_pool_t name = {.mem = (u8 *)__mem,.block_size = __block_size,.size_in_bytes = __size_in_bytes};

#define POOL_DEFINE_STATIC(name,__mem,__block_size,__size_in_bytes)\
_Static_assert(sizeof(__block_size) >= 4, "Pool elements should be at least 4 bytes");\
static bad_pool_t name = {.mem = (u8 *)__mem,.block_size = __block_size,.size_in_bytes = __size_in_bytes};

extern bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, u32 block_size, u32 size_in_bytes);
extern void* pool_alloc(bad_pool_t *pool);
extern void pool_free(bad_pool_t *pool, void *obj);
extern void* gpool_alloc();
extern void gpool_free(void *obj);

#ifdef BAD_RTOS_USE_KHEAP
extern void* kernel_alloc(u32 size);
extern void kernel_free(void *block, u32 size);
#endif

#ifdef BAD_RTOS_USE_MUTEX
#define MUTEX_DEFINE(name)\
bad_mutex_t name = {.blockedq = DLIST_INITIALISER(name.blockedq)}

extern bad_rtos_status_t mutex_init(bad_mutex_t *mut);
extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,u32 delay);
extern bad_rtos_status_t mutex_put(bad_mutex_t *mut);
extern bad_rtos_status_t mutex_delete(bad_mutex_t *mut);
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
#define SEM_DEFINE(name,count)\
bad_sem_t name = {.init_flag = 1, .counter = (count), .blockedq = DLIST_INITIALISER(name.blockedq)}

extern bad_rtos_status_t sem_init(bad_sem_t *sem,u32 reset_value);
extern bad_rtos_status_t sem_take(bad_sem_t *sem,u32 delay);
extern bad_rtos_status_t sem_put(bad_sem_t *sem);
extern bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem);
extern bad_rtos_status_t sem_delete(bad_sem_t *sem);
#endif

#ifdef BAD_RTOS_USE_MSGQ
#define MSGQ_DEFINE(name,size)\
_Static_assert(((size) & ((size) - 1)) == 0, "queue size must be a power of 2"); \
_Static_assert(((size) > 2), "queue size must be a power of 2"); \
bad_msg_block_t name##_blocks [size];\
bad_msgq_t name = {.capacity_mask = size - 1,.msgs = name##_blocks,.blockedq = DLIST_INITIALISER(name.blockedq)};

#define MSGQ_DEFINE_STATIC(name,size)\
_Static_assert(((size) & ((size) - 1)) == 0, "queue size must be a power of 2"); \
_Static_assert(((size) > 2), "queue size must be a power of 2"); \
static bad_msg_block_t name##_blocks [size];\
static bad_msgq_t name = {.capacity_mask = size - 1,.msgs = name##_blocks,.blockedq = DLIST_INITIALISER(name.blockedq)};

#ifdef BAD_RTOS_USE_KHEAP
extern bad_rtos_status_t msgq_acquire_allocate(bad_msgq_t *q, u16 capacity);
extern bad_rtos_status_t msgq_release_deallocate(bad_msgq_t *q);
#endif

extern bad_rtos_status_t msgq_acquire(bad_msgq_t *q);
extern bad_rtos_status_t msgq_release(bad_msgq_t *q);
extern bad_rtos_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback, u32 delay);
extern bad_rtos_status_t msgq_post_msg(bad_msgq_t *q, u32 signal, void *args, u32 delay);
extern bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, u32 signal, void *args);
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
#define EVENT_BARRIER_FLAGS_VALID_MASK (0x80000000UL)

#define EVENT_BARRIER_GET_FLAGS(flags) ({\
((flags) & EVENT_BARRIER_FLAGS_VALID_MASK) ? ((flags)^ EVENT_BARRIER_FLAGS_VALID_MASK ) : 0;\
})

#define EVENT_BARRIER_GET_ERROR(flags) ({\
((flags) & EVENT_BARRIER_FLAGS_VALID_MASK) ? BAD_RTOS_STATUS_OK : (flags);\
})

extern bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, u32 count);
extern u32 event_barrier_wait(bad_event_barrier_t *event_barrier, u32 delay);
extern bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier, u32 flag);
extern bad_rtos_status_t event_barrier_fire(bad_event_barrier_t *event_barrier, u32 flag);
extern bad_rtos_status_t event_barrier_delete(bad_event_barrier_t *event_barrier);
#endif

// Helpers****************************************
// Atomics
__attribute__((always_inline))
static inline u32 __ldrex(volatile u32* addr)
{
    u32 res;
    __asm__ volatile ("ldrex %0, %1" : "=r"(res): "Q"(*addr): "memory");
    return res;
}

__attribute__((always_inline))
static inline u32 __strex(u32 val, volatile u32 * addr)
{
    u32 res;
    __asm__ volatile ("strex %0, %2, %1" : "=&r" (res), "=Q" (*addr) : "r" (val));
    return res;
}

__attribute__((always_inline))
static inline u16 __ldrexh(volatile u16* addr)
{
    u32 res;
    __asm__ volatile ("ldrexh %0, %1" : "=r"(res): "Q"(*addr): "memory");
    return res;
}

__attribute__((always_inline))
static inline u32 __strexh(u16 val, volatile u16 * addr)
{
    u32 res;
    __asm__ volatile ("strexh %0, %2, %1" : "=&r" (res), "=Q" (*addr) : "r" (val));
    return res;
}

__attribute__((always_inline))
static inline void __clrex()
{
    __asm__ volatile ("clrex":::"memory");
}

__attribute__((always_inline))
static inline void __dmb()
{
    __asm__ volatile ("dmb":::"memory");
}

__attribute__((always_inline))
static inline void __dsb()
{
    __asm__ volatile ("dsb":::"memory");
}

__attribute__((always_inline))
static inline void __isb()
{
    __asm__ volatile ("isb":::"memory");
}

// Context
static inline __attribute__((always_inline)) u32 __get_ipsr()
{
    u32 res;
    __asm__ volatile("mrs %0, ipsr":"=r"(res));
    return res;
}

static inline u32 in_isr()
{
    return __get_ipsr() != 0;
}

static inline u32 in_task()
{
    return __get_ipsr() == 0;
}

// Misc
#define BAD_CONTAINER_OF(ptr, type, member) \
({ \
_Static_assert(__builtin_types_compatible_p(typeof(*(ptr)), typeof(((type *)0)->member)), \
"Pointer type mismatch in container_of"); \
((type *)( (char *)(ptr) - __builtin_offsetof(type, member) ));\
})

#define DIV_ROUND_UP(divident, divisor) (((divident) + ((divisor) - 1)) / (divisor))

#define VOLATILE_READ(iden) (*((volatile typeof(iden) *)&(iden)))
#define VOLATILE_WRITE(iden,val) (*((volatile typeof(iden) *)&(iden)) = (val))

#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define MIN(arg1,arg2) ({ (arg1) > (arg2) ? (arg2) : (arg1) ;})

#define MAX(arg1,arg2) ({ (arg1) < (arg2) ? (arg2) : (arg1) ;})

static inline u32 find_pow2_order(u32 val, bool round_down)
{
    u32 ret = 0;
    
    if(val > 1)
        ret = 32 - __builtin_clz(val - 1 + round_down) - round_down;
    
    return ret;
}

//Doubly linked list helpers
#define DLIST_INITIALISER(__name) (bad_link_node_t){&(__name), &(__name)}

#define DLIST_DEFINE(__name)\
bad_link_node_t __name = DLIST_INITIALISER(__name)

static inline void dlist_init(bad_link_node_t *list)
{
    list->next = list;
    list->prev = list;
}

static inline void dlist_add_front(bad_link_node_t *list, bad_link_node_t *node)
{
    node->next = list->next;
    node->next->prev = node;
    node->prev = list;
    list->next = node;
}

static inline void dlist_add_back(bad_link_node_t *list, bad_link_node_t *node)
{
    node->prev = list->prev;
    node->prev->next = node;
    node->next = list;
    list->prev = node;
}

static inline void dlist_remove(bad_link_node_t *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = 0;
    node->prev = 0;
}

static inline bool dlist_is_empty(bad_link_node_t *list)
{
    return list == list->next;
}

static inline bad_link_node_t *dlist_pull_front(bad_link_node_t *list)
{
    bad_link_node_t *ret = 0;
    
    if(!dlist_is_empty(list))
    {
        ret = list->next;
        dlist_remove(ret);
    }
    
    return ret;
}

static inline bad_link_node_t *dlist_pull_back(bad_link_node_t *list)
{
    bad_link_node_t *ret = 0;
    
    if(!dlist_is_empty(list))
    {
        ret = list->prev;
        dlist_remove(ret);
    }
    
    return ret;
}

#define dlist_iter_cond(pos, list, cond) \
for((pos) = (list)->next; \
(pos) != (list) && (cond);\
(pos) = (pos)->next)

#define dlist_iter_cond_safe(pos, s, list, cond) \
for((pos) = (list)->next; \
((pos) == (list) ? ((pos) = 0,0) : ((s) = (pos)->next),1) && (cond);\
(pos) = (s))

#define dlist_iter_all(pos, list) dlist_iter_cond(pos,list,1)

#define dlist_iter_all_safe(pos, s, list) dlist_iter_cond_safe(pos,s,list,1)

#define dlist_iter_cond_cast(pos,list,member,cond) \
for((pos) = BAD_CONTAINER_OF((list)->next,typeof(*pos),member);\
(&((pos)->member) == (list) ? ((pos) = 0, 0) : 1) && (cond);\
(pos) = BAD_CONTAINER_OF((pos)->member.next,typeof(*pos),member))

#define dlist_iter_cond_safe_cast(pos, s, list, member, cond) \
for((pos) = BAD_CONTAINER_OF((list)->next,typeof(*pos),member);\
(&((pos)->member) == (list) ? ((pos) = 0, 0) : ((s) = BAD_CONTAINER_OF((pos)->member.next,typeof(*pos),member),1)) && (cond);\
(pos) = (s))

#define dlist_iter_all_cast(pos, list,member)\
dlist_iter_cond_cast(pos,list,member,1)

#define dlist_iter_all_safe_cast(pos, s, list, member)\
dlist_iter_cond_safe_cast(pos, s, list, member,1)

//Lockless list
static inline void llist_add(bad_llink_node_t *list,bad_llink_node_t *node)
{
    u32 headval;
    u32 res;
    
    __asm__ volatile (
                      "1:                \n"
                      "ldr %0, [%2]      \n"
                      "str %0, [%3]      \n"
                      "dmb               \n"
                      "ldrex %1, [%2]    \n"
                      "cmp %0, %1        \n"
                      "bne 1b            \n"
                      "strex %1, %3, [%2]\n"
                      "cmp %1,#0         \n"
                      "bne 1b            \n"
                      : "=&r"(headval),"=&r"(res)
                      : "r"(list),"r"(node)
                      : "cc","memory"
                      );
}

static inline bad_llink_node_t *llist_pull(bad_llink_node_t *list)
{
    u32 nextval;
    u32 res;
    
    bad_llink_node_t *ret;
    
    __asm__ volatile (
                      "1:                \n"
                      "ldrex %0, [%3]    \n"
                      "cbnz %0, 2f       \n"
                      "clrex             \n"
                      "b 3f              \n"
                      "2:                \n"
                      "ldr %1, [%0]      \n"
                      "strex %2, %1, [%3]\n"
                      "cmp %2,#0         \n"
                      "bne 1b            \n"
                      "3:                \n"
                      :"=&l"(ret),"=&r"(nextval),"=&r"(res)
                      : "r"(list)
                      : "cc","memory"
                      );
    return ret;
}

static inline bad_llink_node_t *llist_pull_all(bad_llink_node_t *list)
{
    u32 nextval;
    u32 res;
    
    bad_llink_node_t *ret;
    
    __asm__ volatile (
                      "movs %1,#0        \n"
                      "1:                \n"
                      "ldrex %0, [%3]    \n"
                      "strex %2, %1, [%3]\n"
                      "cmp %2,#0         \n"
                      "bne 1b            \n"
                      "3:                \n"
                      :"=&r"(ret),"=&r"(nextval),"=&r"(res)
                      : "r"(list)
                      : "cc","memory"
                      );
    return ret;
}

//Bitmaps
static inline u32 bmap_toggle_bit(u32 *bmap, u32 pos)
{
    u32 word = pos / 32;
    u32 bit = pos % 32;
    bmap[word] ^= 1UL << bit;
    
    return (bmap[word] & 1UL << bit) != 0;
}

static inline void bmap_set_bit(u32 *bmap, u32 pos)
{
    u32 word = pos / 32;
    u32 bit = pos % 32;
    bmap[word] |= 1UL << bit;
}

static inline void bmap_clear_bit(u32 *bmap, u32 pos)
{
    u32 word = pos / 32;
    u32 bit = pos % 32;
    bmap[word] &= ~(1UL << bit);
}

static inline u32 bmap_test_bit(u32 *bmap, u32 pos)
{
    u32 word = pos / 32;
    u32 bit = pos % 32;
    
    return (bmap[word] & 1UL << bit) != 0; 
}

static inline u32 bmap_ffs(u32 *bmap, u32 bmap_size, u32 pos)
{
    u32 ret = bmap_size;
    if(pos < bmap_size)
    {
        u32 bmap_word_size = DIV_ROUND_UP(bmap_size,32);
        
        u32 word = pos / 32;
        u32 bit = pos % 32;
        
        u32 mask = ~0UL << bit;
        u32 first_word = bmap[word] & mask;
        
        if(first_word)
        {
            ret = __builtin_ctz(first_word);
        }
        else
        {
            for(u32 i = word + 1; i < bmap_word_size; i++)
            {
                if(bmap[i])
                {
                    u32 find = __builtin_ctz(bmap[i]);
                    u32 res_pos = i * 32 + find;
                    
                    ret = MIN(bmap_size,res_pos);
                    break;
                }
            }
        }
    }
    
    return ret;
}

//Helpers for bitmapped dlist (array of list heads + bmap to track occupancy)
//Currently used in buddy alloc and ready queue
static inline void bmap_dlist_add_front(bad_link_node_t *list_arr,
                                        bad_link_node_t *node,
                                        u32 *bmap,
                                        u32 pos)
{
    dlist_add_front(list_arr + pos,node);
    bmap_set_bit(bmap,pos);
}

static inline void bmap_dlist_add_back(bad_link_node_t *list_arr,
                                       bad_link_node_t *node,
                                       u32 *bmap,
                                       u32 pos)
{
    dlist_add_back(list_arr + pos,node);
    bmap_set_bit(bmap,pos);
}

static inline void bmap_dlist_remove(bad_link_node_t *list_arr,
                                     bad_link_node_t *node,
                                     u32 *bmap,
                                     u32 pos)
{
    bad_link_node_t *list = list_arr + pos;
    
    dlist_remove(node);
    
    if(dlist_is_empty(list))
        bmap_clear_bit(bmap,pos);
}

static inline bad_link_node_t * bmap_dlist_pull_front(bad_link_node_t *list_arr,
                                                      u32 *bmap,
                                                      u32 pos)
{
    bad_link_node_t *list = list_arr + pos;
    bad_link_node_t *ret = list->next;
    
    bmap_dlist_remove(list_arr,ret,bmap,pos);
    
    return ret;
}

static inline bad_link_node_t * bmap_dlist_pull_back(bad_link_node_t *list_arr,
                                                     u32 *bmap,
                                                     u32 pos)
{
    bad_link_node_t *list = list_arr + pos;
    bad_link_node_t *ret = list->prev;
    
    bmap_dlist_remove(list_arr,ret,bmap,pos);
    
    return ret;
}

//Iter sections
/* 
 * The section name MUST be a valid C identifier (letters, numbers, underscores).
 * 'used' prevents the compiler from discarding it if unreferenced.
 * 'aligned' prevents padding issues when iterating.
 */
#define BAD_ITER_SECTION_MEMBER(section_name, type, var_name) \
__attribute__((used, section(#section_name), aligned(_Alignof(type)))) \
const type var_name

#define BAD_ITER_SECTION_EXTERN(section_name,type) \
extern const type __start_##section_name;\
extern const type __stop_##section_name

#define BAD_ITER_SECTION_ITER_ALL(section_name,type,pos)\
for((pos) = &__start_##section_name; (pos) < &__stop_##section_name; (pos)++)

#ifdef BAD_RTOS_IMPLEMENTATION

#define BAD_RTOS_PRIO_COUNT BAD_RTOS_MAX_TASKS
typedef enum 
{
    BAD_ISR_OP_MSGQ_WAKE,
    BAD_ISR_OP_SEM_PUT,
    BAD_ISR_OP_TASK_DELAY_CANCEL,
    BAD_ISR_OP_TASK_UNBLOCK,
    BAD_ISR_OP_EVENT_BARRIER_WAKE
} bad_isr_op_t;

typedef struct bad_isr_op_obj bad_isr_op_obj_t;
struct bad_isr_op_obj
{
    bad_isr_op_obj_t * volatile next;
    bad_isr_op_t op_kind;
    void *arg;
    u32 pad;
};

typedef struct 
{
    bad_isr_op_obj_t * volatile tail;
    bad_isr_op_obj_t * volatile head;
    bad_isr_op_obj_t stub;
} bad_isr_q_t;

typedef struct 
{
    //NOTE: DONT MOVE AROUND!!!
    volatile u32 ticks;
    
    bad_tcb_t *curr;
    bad_tcb_t *next;
    
    bad_link_node_t delayq;
    
    u8 is_running;
    //
    
    bad_link_node_t blockedq;
    
    u32 ready_bmap;
    bad_link_node_t readyq[BAD_RTOS_PRIO_COUNT];
    
    bad_isr_q_t isrq;
    
    u32 irq_bmap[DIV_ROUND_UP(BAD_RTOS_IRQ_COUNT,32)];
    
} bad_kernel_cb_t;

typedef struct bitmap_slab_cb
{
    u32 free_bitmap;
    bad_tcb_t node_arr[BAD_RTOS_MAX_TASKS];
} tcb_bitmap_slab_t;

typedef enum 
{
    BAD_SYSTICK_TIMEFRAME_PENDING = 0x1,
    BAD_SYSTICK_DELAY_WAKE_PENDING = 0x2,
    BAD_SYSTICK_BOTH = BAD_SYSTICK_DELAY_WAKE_PENDING|BAD_SYSTICK_TIMEFRAME_PENDING//0x3
} bad_systick_status_t;

#ifdef BAD_RTOS_USE_KHEAP
typedef struct 
{
    u8* heap;
    u32 heads_bmap;
    u32 max_order;
    u32 min_order;
    bad_link_node_t* free_list;
    u32* bmap;
} bad_buddy_t;
#define BUDDY_BITMAP_SIZE(max_order,min_order)\
DIV_ROUND_UP(1UL << ((max_order) - (min_order)),32) // bits required = (2 ^ max_order - min_order) - 1, to get the words divide by 32 and round up

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
#if KMAX_ORDER < KMIN_ORDER
#error "Its called max order for a reason"
#endif
#endif 
#define KHEAP_SIZE (1 << KMAX_ORDER)
#define KFREE_LIST_SIZE (KMAX_ORDER - KMIN_ORDER + 1)
#endif

#define BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES (BAD_RTOS_GLOBAL_POOL_SIZE * sizeof(bad_isr_op_obj_t))

_Static_assert( 1
#ifdef BAD_RTOS_USE_MUTEX
               && sizeof(bad_isr_op_obj_t) == sizeof(bad_mutex_t)
#endif
#ifdef BAD_RTOS_USE_SEMAPHORE
               && sizeof(bad_isr_op_obj_t) == sizeof(bad_sem_t)
#endif
#ifdef BAD_RTOS_USE_EVENT_BARRIER
               && sizeof(bad_isr_op_obj_t) == sizeof(bad_event_barrier_t)
#endif
               ,"What have i done #1");

_Static_assert( 1
#ifdef BAD_RTOS_USE_MUTEX
               && __builtin_offsetof(bad_mutex_t,blockedq) == 0
#endif
#ifdef BAD_RTOS_USE_MSGQ
               && __builtin_offsetof(bad_msgq_t,blockedq) == 0
#endif
#ifdef BAD_RTOS_USE_SEMAPHORE
               && __builtin_offsetof(bad_sem_t,blockedq) == 0
#endif
#ifdef BAD_RTOS_USE_EVENT_BARRIER
               && __builtin_offsetof(bad_event_barrier_t,blockedq) == 0
#endif
               ,"What have i done #2");

#ifdef BAD_RTOS_USE_KHEAP
static u8 __attribute__((section(".kheap"))) kheap[KHEAP_SIZE];
static bad_buddy_t __attribute__((section(".kernel_bss")))kernel_buddy;
static bad_link_node_t __attribute__((section(".kernel_bss"))) kfreelist[KFREE_LIST_SIZE];
static u32 __attribute__((section(".kernel_bss"))) kbitmap[BUDDY_BITMAP_SIZE(KMAX_ORDER, KMIN_ORDER)]; 
#endif

static bad_kernel_cb_t __attribute__((section(".kernel_bss"))) kernel_cb;

static tcb_bitmap_slab_t __attribute__((section(".kernel_bss"))) tcbslab;

static u8  __attribute__((aligned(_Alignof(bad_isr_op_obj_t)))) gpool_mem[BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES];
static bad_pool_t gpool; //Not initialsed to prevent isrs from generating events before the kernel has started

volatile u32 preempt_count = 1;

#define IDLE_TASK_PRIO BAD_RTOS_PRIO_COUNT - 1
#define IDLE_TASK_STACK_SIZE 128

u8 idle_stack[IDLE_TASK_STACK_SIZE] __attribute__((section(".static_stacks")));

// Prototypes for asm helpers, for implementation look right above svc_c function, or grep for "ASM stuff"
extern void idle_task(void *);
extern void __first_task_start();
extern void __svc_check_resched();

#ifdef BAD_RTOS_USE_SEMAPHORE
extern bad_rtos_status_t __svc_sem_take(bad_sem_t *sem,u32 delay);
extern bad_rtos_status_t __svc_sem_put(bad_sem_t *sem);
#endif

static inline u32 __attribute__((always_inline)) __modify_basepri(u32 basepri);
static inline void __attribute__((always_inline)) __restore_basepri(u32 basepri);
static inline void __attribute__((always_inline)) __set_control(u32 control);
static inline u32 __attribute__((always_inline)) __get_control();

#define BAD_OPT_BARRIER __asm__ volatile("":::"memory")
#define BAD_RTOS_STATIC static 

#define __TASK_HANDLE_INVALID_HANDLE(error) (bad_task_handle_t){.gen = error, .idx = 0xFFFF}
#define __TASK_HANDLE_IS_VALID(tcb,handle) ({handle.idx && tcb && tcb->generation == handle.gen;})
#ifdef __GCC_HAVE_DWARF2_CFI_ASM
# define BAD_CFI(s) s
#else
# define BAD_CFI(s)
#endif

//Linker script symbols
extern u8 __kernel_bss[];
extern u8 __ekernel_bss[];

extern u8 __static_stacks[];

extern u8 __heap[];

extern u8 __dma_buffs[];

// ///CODE_REPLACE_START
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
    BAD_SCB_BUS_FAULT_INTR = 1,
    BAD_SCB_USAGE_FAULT_INTR  = 2,
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
    __dsb();
}

static inline void __scb_set_core_interrupt_priority(bad_scb_core_interrupt_t intr, bad_scb_interrupt_priority_t prio)
{
    BAD_SCB->SHP[intr] = prio << (8 - BAD_RTOS_PRIO_BITS);
    __dsb();
}

static inline void __scb_set_fpu_permission_level(bad_scb_fpu_permission_t perms)
{
    BAD_SCB->CPACR &= ~(BAD_SCB_CPACR_FPU_MASK);
    BAD_SCB->CPACR |= perms << BAD_SCB_CPACR_FPU_SHIFT;
    __dsb();
    __isb();
} 

static inline void __scb_enable_fault(bad_scb_core_interrupt_t intr)
{
    BAD_SCB->SHCSR |= 1U << (16 + intr);
    __dsb();
}

static inline void __scb_disable_fault(bad_scb_core_interrupt_t intr)
{
    BAD_SCB->SHCSR &= ~(1U << (16 + intr));
    __dsb();
}

static inline void __scb_pend_fault(bad_scb_core_interrupt_t intr)
{
    static const u8 pend_bit[3] = {13, 14, 12};
    
    BAD_OPT_BARRIER;
    BAD_SCB->SHCSR |= 1U << pend_bit[intr];
    __dsb();
}

static inline void __scb_clear_fault(bad_scb_core_interrupt_t intr)
{
    static const u8 pend_bit[3] = {13, 14, 12};  
    
    BAD_OPT_BARRIER;
    BAD_SCB->SHCSR &= ~(1U << pend_bit[intr]);
    __dsb();
}

typedef struct
{
    volatile u32 ISER[8U];              
    u32 RESERVED0[24U];
    volatile u32 ICER[8U];               
    u32 RESERVED1[24U];
    volatile u32 ISPR[8U];               
    u32 RESERVED2[24U];
    volatile u32 ICPR[8U];               
    u32 RESERVED3[24U];
    volatile u32 IABR[8U];               
    u32 RESERVED4[56U];
    volatile u8 IP[240U];               
    u32 RESERVED5[644U];
    volatile u32 STIR;                   
} bad_nvic_typedef_t;

#define BAD_NVIC_BASE (0xE000E100UL)

#define BAD_NVIC ((bad_nvic_typedef_t *) BAD_NVIC_BASE)

static inline void __nvic_enable_interrupt(u32 intrnum)
{
    u8 ISER_idx = intrnum >> 5; //deside the register by dividing by 32 
    u32 ISER_intr_mask = 1 << (intrnum & 0x1F); // the remainder will be the bit number to which we should write
    BAD_OPT_BARRIER;
    BAD_NVIC->ISER[ISER_idx] = ISER_intr_mask;
    __dsb();
}

static inline void __nvic_disable_interrupt(u32 intrnum)
{
    u8 ICER_idx = intrnum >> 5;
    u32 ICER_intr_mask = 1 << (intrnum & 0x1F);
    BAD_OPT_BARRIER;
    BAD_NVIC->ICER[ICER_idx] = ICER_intr_mask;
    __dsb();
} 

static inline void __nvic_pend_interrupt(u32 intrnum)
{
    u8 ISPR_idx = intrnum >> 5;
    u32 ISPR_intr_mask = 1 << (intrnum & 0x1F);
    BAD_OPT_BARRIER;
    BAD_NVIC->ISPR[ISPR_idx] = ISPR_intr_mask;
    __dsb();
}

static inline void __nvic_clear_interrupt(u32 intrnum)
{
    u8 ICPR_idx = intrnum >> 5;
    u32 ICPR_intr_mask = 1 << (intrnum & 0x1F);
    BAD_OPT_BARRIER;
    BAD_NVIC->ICPR[ICPR_idx] = ICPR_intr_mask;
    __dsb();
}

static inline void __nvic_set_interrupt_priority(u32 intrnum, u8 prio)
{
    BAD_NVIC->IP[intrnum] = prio << (8 - BAD_RTOS_PRIO_BITS);
    __dsb();
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
#define BAD_FPU ((bad_fpu_typedef_t *)BAD_FPU_BASE)

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
    u32 order = find_pow2_order(bytes,round_up);
    
    return (order - 1) << 1;
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
    BAD_MPU->RASR = __mpu_find_size((u32)__static_stacks - (u32)__kernel_bss,false)|
        BAD_MPU_RASR_AP_PRIV_RO_UNPRIV_FAULT |
        BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE |
        BAD_MPU_TEXSCB_NORMAL_SHAREABLE |
        BAD_MPU_RASR_XN;
    
    __mpu_enable_with_default_map();
}

static inline bad_rtos_status_t __mpu_translate_settings(bad_tcb_t *tcb, const bad_task_descr_t *descr)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    //Stack region
    {
        u32 addr_cast = (u32)tcb->stack;
        bad_mpu_region_t *stack_region = &tcb->regions[0];
        stack_region->__reg0 = addr_cast | BAD_MPU_RBAR_VALID | BAD_MPU_RBAR_REGION(4);
        stack_region->__reg1 = BAD_RTOS_STACK_RASR;
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
                
                u32 size = user_region->size > 32 ? user_region->size : 32;
                u32 size_msb = find_pow2_order(size,false);
                
                {
                    u32 addr_lowest_bit = __builtin_ctz(addr_cast);
                    
                    if(!user_region->addr || addr_lowest_bit < size_msb)
                    {
                        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
                        goto exit;
                    }
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
                    region->__reg1 = ((size_msb - 1) << 1) | reg1_mask | BAD_MPU_RASR_ENABLE;
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

// ///CODE_REPLACE_END

// cast sp to this
typedef struct
{
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 r10;
    u32 r11;
    u32 exc_return;
    
    u32 r0;
    u32 r1;
    u32 r2;
    u32 r3;
    u32 r12;
    u32 lr;
    u32 pc;
    u32 xpsr;
} cortex_m_slim_exc_frame_t;

typedef struct
{
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 r9;
    u32 r10;
    u32 r11;
    u32 exc_return;
    
    u32 fpu_top[16]; //16 -> 31
    u32 r0;
    u32 r1;
    u32 r2;
    u32 r3;
    u32 r12;
    u32 lr;
    u32 pc;
    u32 xpsr;
    u32 fpu_bot[16]; //0 -> 15
    u32 fpcsr;
    u32 pad;
} cortex_m_ext_exc_frame_t;

#ifdef BAD_RTOS_USE_MPU
# define __GUARD_SIZE (32)
#else
# define __GUARD_SIZE (0)
#endif

#ifdef BAD_RTOS_USE_FPU
_Static_assert(BAD_RTOS_MIN_STACK_SIZE >= sizeof(cortex_m_ext_exc_frame_t) + __GUARD_SIZE, "Frame size mismatch");
#else
_Static_assert(BAD_RTOS_MIN_STACK_SIZE >= sizeof(cortex_m_slim_exc_frame_t) + __GUARD_SIZE, "Frame size mismatch");
#endif

static inline void __arm_write_retval(bad_tcb_t *tcb, u32 val)
{
    cortex_m_slim_exc_frame_t *frame = (cortex_m_slim_exc_frame_t *)tcb->sp;
    
    if(frame->exc_return & 0x10) // Non FPU frame
    {
        frame->r0 = val;
    }
    else
    {
        cortex_m_ext_exc_frame_t *ex_frame = (cortex_m_ext_exc_frame_t *)tcb->sp;
        ex_frame->r0 = val;
    }
}

static inline u32 __arm_get_gpreg(bad_tcb_t *tcb,u32 reg_num)
{
    cortex_m_slim_exc_frame_t *frame = (cortex_m_slim_exc_frame_t *)tcb->sp;
    u32 ret = 0;
    
    if(reg_num > 12)
        __builtin_trap();
    
    if(reg_num < 4 || reg_num == 12)
    {
        u32 off = reg_num == 12 ? 4 : reg_num;
        
        if(frame->exc_return & 0x10)
        {
            ret = *(&frame->r0 + off);
        }
        else
        {
            cortex_m_ext_exc_frame_t *ex_frame = (cortex_m_ext_exc_frame_t *)tcb->sp;
            ret = *(&ex_frame->r0 + off);
        }
    }
    else 
    {
        ret = *(&frame->r4 + reg_num - 4);
    }
    
    return ret;
}

// Memory helpers
#ifdef BAD_RTOS_USE_KHEAP

static inline u32 __buddy_toggle_bmap(bad_buddy_t *cb, u8 *block, u32 order)
{
    u32 idx = cb->max_order - order;
    
    u32 offset_from_base = block - cb->heap;
    
    u32 level = (1 << (idx - 1)) - 1;
    u32 offset = offset_from_base >> (order + 1);
    u32 bmapidx = level + offset;
    
    u32 ret = bmap_toggle_bit(cb->bmap, bmapidx);
    
    return ret;
}

void  __buddy_init(bad_buddy_t *cb,
                   u8 *heap, 
                   bad_link_node_t *freelist,
                   u32 min_order, 
                   u32 max_order, 
                   u32 *bmap)
{
    cb->min_order = min_order;
    cb->max_order = max_order;
    cb->heap = heap;
    cb->free_list = freelist;
    cb->bmap = bmap;
    
    for (u32 i = 0; i < max_order - min_order + 1; i++)
    {
        dlist_init(&freelist[i]);
    }
    
    bad_link_node_t *embedded_node = (bad_link_node_t*)cb->heap;
    bmap_dlist_add_front(freelist,embedded_node,&cb->heads_bmap,0);
}

static void* __buddy_alloc(bad_buddy_t *cb,u32 order)
{
    void *ret = 0;
    
    if(order <= cb->max_order)
    {
        u32 picked_idx = 0;
        u32 idx = cb->max_order  - order;
        
        {
            u32 order_mask = (1 << (idx + 1)) - 1;
            u32 masked = cb->heads_bmap & order_mask;
            
            if(!masked)
                goto exit;
            
            picked_idx = 31 - __builtin_clz(cb->heads_bmap & order_mask);
        }
        
        u8 *block_for_split = (u8 *)bmap_dlist_pull_front(cb->free_list,&cb->heads_bmap,picked_idx);
        
        u32 splited_block_size = 1UL << (cb->max_order - picked_idx - 1);
        
        if(picked_idx)
            __buddy_toggle_bmap(cb,block_for_split,cb->max_order - picked_idx);
        
        for(; picked_idx < idx; picked_idx++)
        {
            u32 splited_idx = picked_idx + 1;
            
            bad_link_node_t *unused_block = (bad_link_node_t *)(block_for_split + splited_block_size);
            
            bmap_dlist_add_front(cb->free_list,unused_block,&cb->heads_bmap, splited_idx);
            
            u32 split_order = cb->max_order - splited_idx;
            __buddy_toggle_bmap(cb,(u8 *)unused_block,split_order);
            
            splited_block_size >>= 1UL;
        }
        
        ret = block_for_split;
    }
    
    exit:
    return ret;
}

static void __buddy_free(bad_buddy_t *cb,void *block, u32 order)
{
    if(order <= cb->max_order)
    {
        u32 curr_order = order; 
        void *curr_block = block;
        u32 idx = 0;
        
        while((idx = cb->max_order - curr_order))
        {
            if(__buddy_toggle_bmap(cb,curr_block,curr_order))
                break;
            
            u32 offset_from_base = (u8 *)curr_block - cb->heap;
            u32 buddy_bitmask = 1ULL << curr_order;
            u32 buddy_offset = offset_from_base ^ buddy_bitmask;
            u32 parent_offset = offset_from_base &(~buddy_bitmask);
            void *buddy_addr = (void*)(cb->heap + buddy_offset);
            void *parent_addr = (void*)(cb->heap + parent_offset); 
            
            bad_link_node_t *buddy = (bad_link_node_t *)buddy_addr;
            bmap_dlist_remove(cb->free_list,buddy,&cb->heads_bmap,idx);
            
            curr_order++;
            curr_block = parent_addr;
        }
        
        bad_link_node_t *final_block = (bad_link_node_t*)curr_block;
        
        bmap_dlist_add_front(cb->free_list,final_block,&cb->heads_bmap,idx);
    }
}

BAD_RTOS_STATIC void* __kernel_alloc(u32 size)
{
    u32 closest_order = find_pow2_order(size,false);
    
    if(closest_order < kernel_buddy.min_order)
        closest_order = kernel_buddy.min_order;
    
    return __buddy_alloc(&kernel_buddy,closest_order);
}

BAD_RTOS_STATIC void __kernel_free(void *block,u32 size)
{
    u32 closest_order = find_pow2_order(size,false);
    
    if(closest_order < kernel_buddy.min_order)
        closest_order = kernel_buddy.min_order;
    
    __buddy_free(&kernel_buddy,block,closest_order );
}

#endif

BAD_RTOS_STATIC void __tcb_queue_slab_init()
{
#if BAD_RTOS_MAX_TASKS < 32
    tcbslab.free_bitmap = (1UL << (BAD_RTOS_MAX_TASKS)) - 1;
#else
    tcbslab.free_bitmap = UINT32_MAX;
#endif
}

BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_alloc()
{
    bad_tcb_t *ret = 0;
    
    if(tcbslab.free_bitmap)
    {
        u32 block_idx = bmap_ffs(&tcbslab.free_bitmap,BAD_RTOS_MAX_TASKS,0);
        bmap_clear_bit(&tcbslab.free_bitmap,block_idx);
        
        ret = tcbslab.node_arr + block_idx;
    }
    
    return ret;
}

BAD_RTOS_STATIC u32 __tcb_slab_get_idx_from_ptr(bad_tcb_t *block)
{
    u32 ret = BAD_RTOS_MAX_TASKS + 1;
    
    if(tcbslab.node_arr <= block &&
       tcbslab.node_arr + BAD_RTOS_MAX_TASKS > block)
        ret = block - tcbslab.node_arr;
    
    return ret;
}

BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_get_ptr_from_idx(u32 idx)
{
    bad_tcb_t *ret = 0;
    
    if(idx < BAD_RTOS_MAX_TASKS)
        ret = tcbslab.node_arr + idx;
    
    return ret; 
}

BAD_RTOS_STATIC void __tcb_slab_free(bad_tcb_t *tcb)
{
    u32 block_idx = __tcb_slab_get_idx_from_ptr(tcb); 
    
    if(block_idx < BAD_RTOS_MAX_TASKS)
    {
        tcb->generation++;
        bmap_set_bit(&tcbslab.free_bitmap,block_idx);
    }
}

bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, u32 block_size, u32 size_in_bytes){
    if(!pool || !mem || !block_size ||!size_in_bytes || size_in_bytes % block_size)
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    
    pool->mem = mem;
    pool->block_size = block_size;
    pool->freelist.next = 0;
    pool->curr = 0;
    BAD_OPT_BARRIER;
    
    pool->size_in_bytes = size_in_bytes;
    return BAD_RTOS_STATUS_OK;
}

void* pool_alloc(bad_pool_t *pool)
{
    void *res = llist_pull(&pool->freelist);
    
    if(!res)
    {
        u32 curr = 0;
        do
        {
            curr = __ldrex(&pool->curr);
            
            if(curr >= pool->size_in_bytes)
                goto exit;
            
        }
        while(__strex(curr + pool->block_size,&pool->curr));
        
        res = pool->mem + curr;
    }
    
    exit:
    return res;
}

void pool_free(bad_pool_t *pool,void *obj)
{
    u8 *cmp_ptr = obj;
    
    if(pool->mem > cmp_ptr || pool->mem + pool->size_in_bytes <= cmp_ptr)
        __builtin_trap();
    
    llist_add(&pool->freelist,obj);
}

void* gpool_alloc(){
    return pool_alloc(&gpool);
}

void gpool_free(void *obj){
    pool_free(&gpool,obj);
}

//Scheduling helpers
BAD_RTOS_STATIC void __prio_list_enqueue(bad_link_node_t *q,bad_tcb_t *tcb, bad_rtos_misc_t target)
{
    bad_tcb_t *traverse = 0;
    bad_link_node_t *prev_node = q;
    
    dlist_iter_cond_cast(traverse,q,qnode, traverse->raised_priority <= tcb->raised_priority)
    {
        prev_node = &traverse->qnode;
    }
    
    dlist_add_front(prev_node,&tcb->qnode);
    
    tcb->misc = target;
}

BAD_RTOS_STATIC bad_tcb_t* __prio_list_dequeue_head(bad_link_node_t *q)
{
    bad_tcb_t *ret = 0;
    
    bad_link_node_t *head = dlist_pull_front(q);
    
    if(head)
    {
        ret = BAD_CONTAINER_OF(head,bad_tcb_t,qnode); 
    }
    
    return ret;
}

#ifdef BAD_RTOS_USE_MUTEX  

#ifndef BAD_RTOS_MUTEX_SIMPLE_PI

BAD_RTOS_STATIC void __prio_list_resort(bad_link_node_t *q, bad_tcb_t *tcb, bool deboost)
{
    bad_link_node_t *pos = deboost ? tcb->qnode.next : tcb->qnode.prev;
    bad_link_node_t *start = pos;
    
    while(pos != q)
    {
        bad_tcb_t *pos_tcb = BAD_CONTAINER_OF(pos, bad_tcb_t, qnode);
        s32 diff = pos_tcb->raised_priority - tcb->raised_priority;
        diff = deboost ? diff : diff * -1;
        
        if(diff >= 0 + deboost)
            break;
        
        pos = deboost ? pos->next : pos->prev;
    }
    
    if(pos != start)
    {
        dlist_remove(&tcb->qnode);
        
        if(deboost)
            dlist_add_back(pos,&tcb->qnode);
        else
            dlist_add_front(pos, &tcb->qnode);
    }
}

#endif

BAD_RTOS_STATIC bad_rtos_status_t __readyq_dequeue(bad_tcb_t *tcb)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(tcb->misc == BAD_RTOS_MISC_READYQ_MEMBER)
        bmap_dlist_remove(kernel_cb.readyq,
                          &tcb->qnode,
                          &kernel_cb.ready_bmap,
                          tcb->raised_priority);
    else
        ret = BAD_RTOS_STATUS_WRONG_Q;
    
    return ret;
}

#endif

BAD_RTOS_STATIC void __readyq_enqueue(bad_tcb_t *tcb)
{
    bmap_dlist_add_back(kernel_cb.readyq,
                        &tcb->qnode,
                        &kernel_cb.ready_bmap,
                        tcb->raised_priority);
    
    tcb->misc = BAD_RTOS_MISC_READYQ_MEMBER;
}

BAD_RTOS_STATIC u32 __get_top_ready_prio()
{
    return bmap_ffs(&kernel_cb.ready_bmap,BAD_RTOS_MAX_TASKS,0);
}

BAD_RTOS_STATIC bad_tcb_t *__readyq_dequeue_head()
{
    u32 top = __get_top_ready_prio();
    bad_link_node_t *tcb_qnode_ptr = 
        bmap_dlist_pull_front(kernel_cb.readyq,&kernel_cb.ready_bmap,top);
    
    return BAD_CONTAINER_OF(tcb_qnode_ptr, bad_tcb_t, qnode);
}

BAD_RTOS_STATIC void __delayq_enqueue(bad_tcb_t *tcb, u32 absolute, bad_rtos_delayq_misc_t delayq_misc)
{
    bad_tcb_t *traverse = 0;
    bad_link_node_t *prev_node = &kernel_cb.delayq;
    u32 compound = 0;
    
    dlist_iter_cond_cast(traverse,&kernel_cb.delayq,delaynode, (compound += traverse->counter) <= absolute)
    {
        prev_node = &traverse->delaynode;
    }
    
    if(traverse)
    {
        compound -= traverse->counter;
        traverse->counter -= absolute - compound;
    }
    
    tcb->delayq_misc = delayq_misc;
    tcb->counter = absolute - compound;
    
    dlist_add_front(prev_node,&tcb->delaynode);
}

BAD_RTOS_STATIC bad_rtos_status_t __delayq_dequeue(bad_tcb_t *tcb)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(tcb->delayq_misc != BAD_RTOS_MISC_NOT_DELAYED)
    {
        bad_link_node_t *tcb_delaynode_ptr = &tcb->delaynode;
        bad_link_node_t *head = &kernel_cb.delayq;
        
        if(tcb_delaynode_ptr->next != head)
        {
            bad_tcb_t *next = BAD_CONTAINER_OF(tcb_delaynode_ptr->next,bad_tcb_t,delaynode);
            next->counter += tcb->counter;
        }
        
        dlist_remove(tcb_delaynode_ptr);
        
        tcb->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED;
        tcb->cbptr = 0;
        tcb->args = 0;
    }
    else
    {
        ret = BAD_RTOS_STATUS_WRONG_Q;
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_tcb_t* __delayq_dequeue_head()
{
    bad_tcb_t *ret = 0;
    
    bad_link_node_t *head = dlist_pull_front(&kernel_cb.delayq);
    
    if(head)
    {
        ret = BAD_CONTAINER_OF(head,bad_tcb_t,delaynode);
        ret->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED; 
    }
    
    return ret;
}

BAD_RTOS_STATIC void __isr_q_push(bad_isr_q_t *q,bad_isr_op_obj_t* msg)
{
    bad_isr_op_obj_t *tail;
    
#ifdef BAD_RTOS_USE_MPU
    u32 key = __mpu_kernel_region_save_unlock();
#endif
    
    msg->next = 0;
    
    do
    {
        tail = (bad_isr_op_obj_t *)__ldrex((volatile u32*)&q->head);
    }
    while(__strex((u32)msg,(volatile u32 *)&q->head));
    
    tail->next = msg;
    
    __dmb();
#ifdef BAD_RTOS_USE_MPU
    __mpu_kernel_region_restore_lock(key);
#endif
}

BAD_RTOS_STATIC bad_isr_op_obj_t *__isr_q_pop(bad_isr_q_t *q)
{
    bad_isr_op_obj_t *next =q->tail->next;
    bad_isr_op_obj_t *tail = q->tail;
    
    if(q->tail == &q->stub)
    {
        if(!next)
            return 0;
        
        q->tail = next;
        tail = next;
        next = next->next;
    }
    
    if(next)
    {
        q->tail = next;
        return tail;
    }
    
    if(q->head != tail)
        return 0; //not possible in the current usecase but nice to have
    
    __isr_q_push(q,&q->stub);
    next = tail->next;
    
    if(!next)
        return 0;//not possible in the current usecase but nice to have
    
    q->tail = next;
    return tail;
}

BAD_RTOS_STATIC bad_rtos_status_t __kernel_notify(bad_isr_op_t op,void *arg)
{
    bad_isr_op_obj_t *message = gpool_alloc();
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(message)
    {
        message->op_kind = op;
        message->arg = arg;
        __dmb();
        
        __isr_q_push(&kernel_cb.isrq,message);
        __scb_trigger_pendsv();
    }
    else
    {
        ret = BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    
    return ret;
}

BAD_RTOS_STATIC void __sched_update(bad_tcb_t *tcb)
{
    kernel_cb.next = tcb;
    tcb->misc = BAD_RTOS_MISC_RUNNING;
}

BAD_RTOS_STATIC void __sched_try_update()
{
    bad_tcb_t *running = kernel_cb.next ? kernel_cb.next : kernel_cb.curr;
    u32 top_ready_prio = __get_top_ready_prio();
    
    if(!preempt_count && top_ready_prio < running->raised_priority)
    {
        __readyq_enqueue(running);
        __sched_update(__readyq_dequeue_head());
    }
}

BAD_RTOS_STATIC void __sched_try_preempt(bad_tcb_t *tcb)
{
    bad_tcb_t *running = kernel_cb.next ? kernel_cb.next : kernel_cb.curr;
    if(!preempt_count && tcb->raised_priority < running->raised_priority)
    {
        __readyq_enqueue(running);
        __sched_update(tcb);
    }
    else
    {
        __readyq_enqueue(tcb);
    } 
}

BAD_RTOS_STATIC u32 * __init_stack(taskptr task, u32 *stacktop, void *args)
{
    //reserve space for a standart frame without fpu
    u32 *sp = stacktop - sizeof(cortex_m_slim_exc_frame_t) / sizeof(u32); 
    
    cortex_m_slim_exc_frame_t *frame = (cortex_m_slim_exc_frame_t *)sp;
    
    frame->r0 = (u32)args;
    frame->r1 = 0x01010101UL;
    frame->r2 = 0x02020202UL;
    frame->r3 = 0x03030303UL;
    frame->r12 = 0x12121212UL;
    frame->lr = 0x0;
    frame->pc = (u32)task|0x1; //thumb code (bit 0 set)
    frame->xpsr = 0x01000000UL; // xPSR (Thumb bit set)
    
    frame->exc_return = 0xFFFFFFFDUL; // (thread mode + PSP + No FPU)
    
    frame->r4 = 0x04040404UL;
    frame->r5 = 0x05050505UL;
    frame->r6 = 0x06060606UL;
    frame->r7 = 0x07070707UL;
    frame->r8 = 0x08080808UL;
    frame->r9 = 0x09090909UL;
    frame->r10 = 0x10101010UL;
    frame->r11 = 0x11111111UL;
    
    return sp;
}

//Core isr api implementations
bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t handle)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(in_isr())
    {
        bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
        
        if(__TASK_HANDLE_IS_VALID(tcb,handle))
            ret = __kernel_notify(BAD_ISR_OP_TASK_UNBLOCK,(void*)handle.val); 
        else
            ret = BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    else
    {
        ret = BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    return ret; 
}

bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t handle)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_WRONG_CONTEXT;
    
    if(in_isr())
    {
        bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
        
        if(__TASK_HANDLE_IS_VALID(tcb,handle) &&
           tcb->delayq_misc == BAD_RTOS_MISC_DELAYQ_MEMBER)
        {
            ret = __kernel_notify(BAD_ISR_OP_TASK_DELAY_CANCEL,(void*)handle.val); 
        }
        else
        {
            ret = BAD_RTOS_STATUS_HANDLE_INVALID;
        }
    }
    else
    {
        ret = BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    return ret;
}

//Core api implenetations
BAD_RTOS_STATIC bad_task_handle_t __task_make(const bad_task_descr_t *args)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    bad_tcb_t *new_task = 0;
    
    if(args->base_priority >= IDLE_TASK_PRIO || !args->ticks_to_change ||
       args->stack_size % BAD_RTOS_STACK_ALIGN || args->stack_size < BAD_RTOS_MIN_STACK_SIZE)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
        goto exit_error;
    }
    
    new_task = __tcb_slab_alloc();
    if(!new_task)
    {
        ret = BAD_RTOS_STATUS_ALLOC_FAIL;
        goto exit_error;
    }
    
#ifdef BAD_RTOS_USE_MSGQ
    if(args->assigned_msgq)
    {
        if(args->assigned_msgq->owner)
        {
            ret = BAD_RTOS_STATUS_NOT_OWNER;
            goto err_free_tcb;
        }
        
        if(!args->assigned_msgq->capacity_mask)
        {
            goto err_free_tcb;
        }
        new_task->msgq_owner = 1;
        args->assigned_msgq->owner = new_task;
    }
    else
    {
        new_task->msgq_owner = 0;
    }
#endif
    
    new_task->stack_size = args->stack_size;
    
    if(args->stack)
    {
        new_task->stack = args->stack;
#ifdef BAD_RTOS_USE_KHEAP
        new_task->dyn_stack = 0;
#endif
    }
    else
    {
#ifdef BAD_RTOS_USE_KHEAP
        new_task->stack = __kernel_alloc(args->stack_size);
        if(!new_task->stack){
            ret = BAD_RTOS_STATUS_ALLOC_FAIL;
            goto err_release_msgq;
        }
        new_task->dyn_stack = 1;
#else
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
        goto err_release_msgq; // No heap and no stack provided
#endif
    }
    
#ifdef BAD_RTOS_USE_MPU
    ret = __mpu_translate_settings(new_task,args);
    if(ret != BAD_RTOS_STATUS_OK)
        goto err_free_stack;
#endif
    
    new_task->entry = args->entry;
    new_task->base_priority = args->base_priority;
    new_task->raised_priority = args->base_priority;
    new_task->ticks_to_change = args->ticks_to_change;
    new_task->counter = args->ticks_to_change;
    
    u32 *stack_top = (u32 *)(new_task->stack + args->stack_size);
    new_task->sp = __init_stack(new_task->entry,stack_top,args->args);
    
    for(u32 i = 0; i < ARRAY_SIZE(kernel_cb.curr->owned_irqs); i++)
    {
        new_task->owned_irqs[i] = -1;
    }
    new_task->irqs_allocated = 0;
    
    if(kernel_cb.is_running)
    {
        __sched_try_preempt(new_task);
    }
    else
    {
        __readyq_enqueue(new_task);
    }
    
    u8 idx = __tcb_slab_get_idx_from_ptr(new_task);
    return (bad_task_handle_t){.idx = idx, .gen = new_task->generation };
    
#ifdef BAD_RTOS_USE_MPU 
    err_free_stack:
#ifdef BAD_RTOS_USE_KHEAP
    if(!args->stack)
        __kernel_free(new_task->stack,args->stack_size);
#endif
#endif
    
    err_release_msgq:
#ifdef BAD_RTOS_USE_MSGQ
    if(args->assigned_msgq)
    {
        args->assigned_msgq->owner = 0;
    }
#endif
    
#ifdef BAD_RTOS_USE_MSGQ
    err_free_tcb:
#endif
    __tcb_slab_free(new_task);
    
    exit_error:
    return __TASK_HANDLE_INVALID_HANDLE(ret);
}

BAD_RTOS_STATIC void  __task_block()
{
    kernel_cb.curr->misc = BAD_RTOS_MISC_BLOCKEDQ_MEMBER;
    dlist_add_front(&kernel_cb.blockedq,&kernel_cb.curr->qnode);
    __sched_update(__readyq_dequeue_head());
}

BAD_RTOS_STATIC bad_rtos_status_t __task_unblock(bad_task_handle_t handle)
{
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(__TASK_HANDLE_IS_VALID(tcb,handle))
    {
        if(tcb->misc == BAD_RTOS_MISC_BLOCKEDQ_MEMBER)
        {
            dlist_remove(&tcb->qnode);
            __sched_try_preempt(tcb);
        }
        else
        {
            ret = BAD_RTOS_STATUS_NOT_BLOCKED;
        }
    }
    else
    {
        ret = BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __task_delay_cancel(bad_task_handle_t handle)
{
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(__TASK_HANDLE_IS_VALID(tcb,handle) && 
       tcb->delayq_misc == BAD_RTOS_MISC_DELAYQ_MEMBER)
    {
        ret = __delayq_dequeue(tcb);
        
        if(ret == BAD_RTOS_STATUS_OK)
        {
            __arm_write_retval(tcb,BAD_RTOS_STATUS_WOKEN);
            
            __sched_try_preempt(tcb);
        }
        else
        {
            ret = BAD_RTOS_STATUS_NOT_DELAYED;
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __task_yield()
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    u32 top_ready_prio = __get_top_ready_prio();
    
    if(!preempt_count && top_ready_prio == kernel_cb.curr->raised_priority)
    {
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
    else
    {
        ret = BAD_RTOS_STATUS_CANT_YIELD;
    }
    
    return ret;
}

BAD_RTOS_STATIC void __task_finish()
{
#ifdef BAD_RTOS_USE_MUTEX
    if(kernel_cb.curr->mutex_count) //trap when task want to finish without releasing mutexes
        __builtin_trap();
#endif 
    
#ifdef BAD_RTOS_USE_MSGQ
    if(kernel_cb.curr->msgq_owner)
        __builtin_trap();
#endif
    
#if defined (BAD_RTOS_USE_KHEAP)
    if(kernel_cb.curr->dyn_stack) //free the dynamically allocated stack 
        __kernel_free(kernel_cb.curr->stack,kernel_cb.curr->stack_size);
#endif
    
    for(u32 i = 0; i < ARRAY_SIZE(kernel_cb.curr->owned_irqs); i++)
    {
        if(kernel_cb.curr->owned_irqs[i] != -1)
        {
            u32 pos = kernel_cb.curr->owned_irqs[i];
            bmap_clear_bit(kernel_cb.irq_bmap,pos);
        }
    }
    
    __sched_update(__readyq_dequeue_head());
    __tcb_slab_free(kernel_cb.curr); //free the the tcb used by task
}

BAD_RTOS_STATIC bad_rtos_status_t __task_delay(u32 delay,cbptr cb, void* args)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(delay)
    {
        kernel_cb.curr->cbptr = cb;
        kernel_cb.curr->args = args;
        
        __delayq_enqueue(kernel_cb.curr,delay,BAD_RTOS_MISC_DELAYQ_MEMBER);
        __sched_update(__readyq_dequeue_head());
    }
    else
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    return ret; 
}

BAD_RTOS_STATIC void __kernel_start()
{
    if(kernel_cb.is_running)
        __builtin_trap();
    
    kernel_cb.is_running = 1;
    
    pool_init(&gpool,gpool_mem,sizeof(bad_isr_op_obj_t),
              BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES);
    
    __set_control(0x1);
    __restore_basepri(0);
    preempt_count = 0;
    
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_SCB_LOWEST_PRIO);
    
    kernel_cb.curr = __readyq_dequeue_head();
    
    __asm__ volatile("b __init_second_stage");
}

//IRQs
BAD_RTOS_STATIC s32 __irq_find(s32 irqn)
{
    u32 slot = 0;
    
    for(slot = 0; slot < ARRAY_SIZE(kernel_cb.curr->owned_irqs); slot++)
    {
        if(irqn == kernel_cb.curr->owned_irqs[slot])
            break;
    }
    
    return slot;
}

BAD_RTOS_STATIC u32 __irq_check(s32 irqn)
{
    u32 ret = 0;
    
    if(irqn != -1)
    {
        u32 slot = __irq_find(irqn);
        ret = slot != ARRAY_SIZE(kernel_cb.curr->owned_irqs); 
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_acquire(s32 irqn)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    u32 pos = irqn + 12;
    
    if(kernel_cb.curr->irqs_allocated == ARRAY_SIZE(kernel_cb.curr->owned_irqs))
    {
        ret = BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    else if(irqn < -12 || 
            pos == BAD_SCB_SVC_INTR || 
            pos == BAD_SCB_PENDSV_INTR ||
            pos == BAD_SCB_SYSTICK_INTR ||
            pos >= BAD_RTOS_IRQ_COUNT)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else
    {
        if(bmap_test_bit(kernel_cb.irq_bmap,pos))
            ret = BAD_RTOS_STATUS_ALLOC_FAIL;
        else
            bmap_set_bit(kernel_cb.irq_bmap,pos);
    }
    
    if(ret == BAD_RTOS_STATUS_OK)
    {
        u32 slot = __irq_find(-1);
        
        if(slot == ARRAY_SIZE(kernel_cb.curr->owned_irqs))
            __builtin_unreachable();
        
        kernel_cb.curr->owned_irqs[slot] = pos;
        kernel_cb.curr->irqs_allocated++;
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_enable(s32 irqn)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    s32 real_irqn = irqn + 12;
    
    if(!__irq_check(real_irqn))
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        if(irqn < 0)
        {
            if(real_irqn < 3)
                __scb_enable_fault(real_irqn);
            else
                ret = BAD_RTOS_STATUS_BAD_PARAMETERS; //TODO: other interrupts NIY
        }
        else
        {
            __nvic_enable_interrupt(irqn);
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_disable(s32 irqn)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    s32 real_irqn = irqn + 12;
    
    if(!__irq_check(real_irqn))
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        if(irqn < 0)
        {
            if(real_irqn < 3)
                __scb_disable_fault(real_irqn);
            else
                ret = BAD_RTOS_STATUS_BAD_PARAMETERS; //TODO: other interrupts NIY
        }
        else
        {
            __nvic_disable_interrupt(irqn);
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_pend(s32 irqn)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    s32 real_irqn = irqn + 12;
    
    if(!__irq_check(real_irqn))
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        if(irqn < 0)
        {
            if(real_irqn < 3)
                __scb_pend_fault(real_irqn);
            else
                ret = BAD_RTOS_STATUS_BAD_PARAMETERS; //TODO: other interrupts NIY
        }
        else
        {
            __nvic_pend_interrupt(irqn);
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_clear(s32 irqn)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    s32 real_irqn = irqn + 12;
    
    if(!__irq_check(real_irqn))
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else 
    {
        if(irqn < 0)
        {
            if(real_irqn < 3)
                __scb_clear_fault(real_irqn);
            else
                ret = BAD_RTOS_STATUS_BAD_PARAMETERS; //TODO: other interrupts NIY
        }
        else
        {
            __nvic_clear_interrupt(irqn);
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_set_prio(s32 irqn, u8 prio)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    s32 real_irqn = irqn + 16;
    
    if(!__irq_check(real_irqn))
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    } 
    else if((1U << BAD_RTOS_PRIO_BITS) - 1 < prio)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else
    {
        if(irqn < 0)
        {
            if(real_irqn < 3)
                __scb_set_core_interrupt_priority(real_irqn,prio);
            else
                ret = BAD_RTOS_STATUS_BAD_PARAMETERS; //TODO: other interrupts NIY
        }
        else
        {
            __nvic_set_interrupt_priority(irqn,prio);
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __irq_release(s32 irqn)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    s32 real_irqn = irqn + 16;
    
    if(real_irqn == -1)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else
    {
        u32 slot = __irq_find(real_irqn);
        
        if(slot == ARRAY_SIZE(kernel_cb.curr->owned_irqs))
        {
            ret = BAD_RTOS_STATUS_NOT_OWNER;
        }
        else
        {
            u32 pos = kernel_cb.curr->owned_irqs[slot];
            
            bmap_clear_bit(kernel_cb.irq_bmap,pos);
            
            kernel_cb.curr->owned_irqs[slot] = -1;
            kernel_cb.curr->irqs_allocated--;
        }
    }
    
    return ret;
}

void preempt_disable()
{
    if(in_isr())
        __builtin_trap();
    
    preempt_count++;
}

void preempt_enable()
{
    if(!preempt_count || in_isr())
        __builtin_trap();
    
    preempt_count--;
    
    if(!preempt_count)
        __svc_check_resched();
}

// Startup code
BAD_RTOS_STATIC void __kernel_sections_init()
{
    u32 *src = (u32 *)__kernel_bss;
    u32 *end = (u32 *)__ekernel_bss;
    
    while (src < end)
    {
        *src++ = 0; 
    }
}

BAD_RTOS_STATIC void __interrupt_init()
{
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_SCB_PRIO0);
    __scb_set_core_interrupt_priority(BAD_SCB_PENDSV_INTR, BAD_SCB_LOWEST_PRIO);
}

BAD_RTOS_STATIC void __tcbqs_init()
{
    for (u32 i = 0; i < BAD_RTOS_PRIO_COUNT; i++)
    {
        dlist_init(&kernel_cb.readyq[i]);
    }
    
    dlist_init(&kernel_cb.blockedq);
    dlist_init(&kernel_cb.delayq);
}

BAD_RTOS_STATIC void __irq_q_init()
{
    kernel_cb.isrq.head = &kernel_cb.isrq.stub;
    kernel_cb.isrq.tail = &kernel_cb.isrq.stub;
}

BAD_RTOS_STATIC void __idle_task_init()
{
    bad_tcb_t *idle_tcb = __tcb_slab_alloc(); //always idx 0
    idle_tcb->stack = idle_stack;
    idle_tcb->stack_size = IDLE_TASK_STACK_SIZE;
    idle_tcb->base_priority = IDLE_TASK_PRIO;
    idle_tcb->raised_priority = IDLE_TASK_PRIO;
    idle_tcb->ticks_to_change = UINT32_MAX;
    idle_tcb->entry = idle_task;
    idle_tcb->counter = UINT32_MAX;
#ifdef BAD_RTOS_USE_MPU
    __mpu_translate_settings(idle_tcb,0);
#endif
    idle_tcb->sp = __init_stack(idle_task, (u32 *)idle_stack +(IDLE_TASK_STACK_SIZE/sizeof(u32)),0);
    __readyq_enqueue(idle_tcb);
}

// declaration for user init function
extern bad_rtos_status_t bad_user_init();

void bad_rtos_start()
{
    __restore_basepri(1 << (8 - BAD_RTOS_PRIO_BITS));
    __kernel_sections_init();
    __tcb_queue_slab_init();
    __irq_q_init();
    __tcbqs_init();
    __interrupt_init();
    __idle_task_init();
#ifdef BAD_RTOS_USE_KHEAP
    __buddy_init(&kernel_buddy, kheap, kfreelist, KMIN_ORDER, KMAX_ORDER, kbitmap);
#endif
#ifdef BAD_RTOS_USE_MPU
    __mpu_default_init();
#endif
#ifdef BAD_RTOS_USE_FPU 
    __scb_set_fpu_permission_level(BAD_SCB_FPU_FULL_ACCESS);
#endif
#if defined(BAD_RTOS_USE_FPU) && defined(BAD_RTOS_FPU_DEFAULT_SETTINGS)
    __fpu_init(BAD_RTOS_FPU_SETTINGS);
#endif
    
    if(bad_user_init() == BAD_RTOS_STATUS_OK)
        __first_task_start();
    else
        __restore_basepri(0);
}

//Synchro helpers
BAD_RTOS_STATIC bad_tcb_t* __synchro_wake(bad_link_node_t *q,cbptr cb,bad_rtos_status_t status)
{
    bad_tcb_t *tcb = __prio_list_dequeue_head(q);
    
    if(tcb)
    {
        if(tcb->cbptr == cb)
            __delayq_dequeue(tcb);
        
        __arm_write_retval(tcb,status);
        
        tcb->blocked_on = 0;
        
        __sched_try_preempt(tcb);
    }
    
    return tcb;
}

BAD_RTOS_STATIC void __synchro_wake_all(bad_link_node_t *q,cbptr cb, u32 status)
{
    bad_tcb_t *traverse = 0;
    bad_tcb_t *safe = 0;
    
    dlist_iter_all_safe_cast(traverse,safe,q,qnode)
    {
        __arm_write_retval(traverse,status);
        
        if(traverse->cbptr == cb)
            __delayq_dequeue(traverse);
        
        dlist_remove(&traverse->qnode);
        
        traverse->blocked_on = 0;
        
        __readyq_enqueue(traverse);
    }
    
    __sched_try_update();
}

BAD_RTOS_STATIC bad_rtos_status_t __synchro_block(bad_link_node_t *q, cbptr cb, u32 delay, bad_rtos_misc_t misc)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(delay == UINT32_MAX)
    {
        ret = BAD_RTOS_STATUS_WOULD_BLOCK;
    }
    else if(delay != UINT32_MAX && preempt_count)
    {
        ret = BAD_RTOS_STATUS_SCHED_LOCKED;
    }
    else
    {
        if(delay)
        {
            kernel_cb.curr->args = q; //every synchro obj has blockedq as first element
            kernel_cb.curr->cbptr = cb;
            __delayq_enqueue(kernel_cb.curr,delay,BAD_RTOS_MISC_DELAYQ_SYNCHRO);
        }
        
        kernel_cb.curr->blocked_on = q;
        __prio_list_enqueue(q,kernel_cb.curr, misc);
        __sched_update(__readyq_dequeue_head());
    }
    
    return ret;
}

// Synchro objects api implenetations
#ifdef BAD_RTOS_USE_MSGQ

BAD_RTOS_STATIC void __msgq_timeout_cb(bad_task_handle_t handle ,void *msgq)
{
    (void)msgq;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    dlist_remove(&tcb->qnode);
    
    __arm_write_retval(tcb,BAD_RTOS_STATUS_TIMEOUT);
}

#ifdef BAD_RTOS_USE_KHEAP

BAD_RTOS_STATIC bad_rtos_status_t __msgq_acquire_allocate(bad_msgq_t *q,u32 capacity)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!q || q->capacity_mask ||
       capacity < 2 || (capacity & (capacity - 1)))
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(q->owner)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        q->msgs = __kernel_alloc(capacity * sizeof(bad_msg_block_t));
        if(q->msgs)
        {
            kernel_cb.curr->msgq_owner++;
            q->owner = kernel_cb.curr;
            q->dynamic = 1;
            q->blockedq = DLIST_INITIALISER(q->blockedq);
            q->head = q->tail = 0;
            BAD_OPT_BARRIER;
            
            q->capacity_mask = capacity - 1;
        }
        else
        {
            ret = BAD_RTOS_STATUS_ALLOC_FAIL;
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __msgq_release_deallocate(bad_msgq_t *q)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!q || !q->dynamic)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(q->owner != kernel_cb.curr)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        kernel_cb.curr->msgq_owner--;
        u32 capacity = q->capacity_mask + 1;
        
        q->capacity_mask = 0;
        BAD_OPT_BARRIER;
        
        __kernel_free(q->msgs,capacity * sizeof(bad_msg_block_t));
        __synchro_wake_all(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_DELETED);
        
        *q = (bad_msgq_t){0};
    }
    
    return ret;
}

#endif

BAD_RTOS_STATIC bad_rtos_status_t __msgq_acquire(bad_msgq_t *q)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!q)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(q->owner)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        q->owner = kernel_cb.curr;
        kernel_cb.curr->msgq_owner++;
    }
    
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __msgq_release(bad_msgq_t *q)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!q)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(q->owner != kernel_cb.curr)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        q->owner = 0;
        kernel_cb.curr->msgq_owner--;
        
        q->atomic_update = 0;
        
        __synchro_wake_all(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_DELETED);
    }
    
    return ret;
}

bad_rtos_status_t __msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback,u32 delay)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!q || !writeback)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!q->capacity_mask)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else if(q->owner != kernel_cb.curr)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else if(q->tail == q->head)
    {
        ret = __synchro_block(&q->blockedq,__msgq_timeout_cb,delay, BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER);
    }
    else
    {
        *writeback = *(q->msgs + q->tail);
        BAD_OPT_BARRIER;
        bad_tcb_t *tcb = __synchro_wake(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_OK);
        
        if(tcb)
        {
            u16 next_tail = (q->tail + 1) & q->capacity_mask;
            u16 next_head = (q->head + 1) & q->capacity_mask;
            
            u32 signal = __arm_get_gpreg(tcb,1);
            void *args = (void *) __arm_get_gpreg(tcb,2);
            
            bad_msg_block_t *block = q->msgs + q->head;
            block->signal = signal;
            block->args = args;
            BAD_OPT_BARRIER;
            
            q->atomic_update = next_head | (next_tail << 16);
        }
        else
        {
            q->tail = (q->tail + 1) & q->capacity_mask;
        }
    }
    
    return ret;
}

BAD_RTOS_STATIC void __msgq_try_wake(bad_msgq_t *q)
{
    if(q->owner && q->owner->blocked_on == &q->blockedq)
    {
        bad_tcb_t *tcb = __synchro_wake(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_OK);
        
        if(tcb)
        {
            bad_msg_block_t *writeback = (bad_msg_block_t *) __arm_get_gpreg(tcb,1);
            *writeback = *(q->msgs + q->tail);
            BAD_OPT_BARRIER;
            
            q->tail = (q->tail + 1) & q->capacity_mask;
        }
    }
}

bad_rtos_status_t __msgq_post_msg(bad_msgq_t *q, u32 signal, void *args,u32 delay)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!q)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!q->capacity_mask)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        u16 next_head = 0;
        u16 head = 0;
        
        do
        {
            head = __ldrexh(&q->head);
            next_head = (head + 1) & q->capacity_mask;
            
            if(q->tail == next_head)
            {
                __clrex();
                ret = __synchro_block(&q->blockedq,__msgq_timeout_cb,delay,BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER);
                goto exit;
            }
        }
        while(__strexh(next_head, &q->head)); 
        
        bad_msg_block_t* block = q->msgs+head;
        
        block->signal = signal;
        block->args = args;
        
        if(head == q->tail)
        {
            __msgq_try_wake(q); 
        }
    }
    
    exit:
    return ret;
}

bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, u32 signal, void *args)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!in_isr())
    {
        ret = BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    else if(!q)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!q->capacity_mask)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        u32 head = 0;
        u32 next_head = 0;
        
        do
        {
            head = __ldrexh(&q->head);
            next_head = (head+1) & q->capacity_mask;
            
            if(q->tail == next_head)
            {
                __clrex();
                ret = BAD_RTOS_STATUS_WOULD_BLOCK;
                goto exit;
            }
        }
        while(__strexh(next_head, &q->head)); 
        
        bad_msg_block_t* block = q->msgs + head;
        
        block->signal = signal;
        block->args = args;
        
        if(q->tail == head) // we may have preempted the consumer mid block, need to check in pendsv
        {
            ret = __kernel_notify(BAD_ISR_OP_MSGQ_WAKE,q);
        }
    }
    
    exit:
    return ret;
}

#endif

#ifdef BAD_RTOS_USE_MUTEX
bad_rtos_status_t mutex_init(bad_mutex_t *mut)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!mut || mut->blockedq.next)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else
    {
        *mut = (bad_mutex_t){0};
        mut->blockedq = DLIST_INITIALISER(mut->blockedq); 
    }
    
    return ret;
}

BAD_RTOS_STATIC void __mutex_timeout_cb(bad_task_handle_t handle, void *mutex)
{
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    dlist_remove(&tcb->qnode);
    
    __arm_write_retval(tcb,BAD_RTOS_STATUS_TIMEOUT);
    
#ifdef BAD_RTOS_MUTEX_SIMPLE_PI
    (void)mutex;
#else
    {
        bad_mutex_t *mut = mutex;
        bad_tcb_t *owner = mut->owner;
        
        if(owner && owner->mutex_count == 1)
        {
            u8 prio = owner->base_priority;
            
            if(!dlist_is_empty(&mut->blockedq))
            {
                bad_tcb_t *head = BAD_CONTAINER_OF(mut->blockedq.next,bad_tcb_t,qnode);
                prio = MIN(prio,head->raised_priority);
            }
            
            if(prio != owner->raised_priority)
            {
                switch(owner->misc)
                {
                    case BAD_RTOS_MISC_READYQ_MEMBER:
                    {
                        __readyq_dequeue(owner);
                        owner->raised_priority = prio;
                        __readyq_enqueue(owner);
                    }break;
                    
                    case BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER:
                    case BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER:
                    {
                        owner->raised_priority = prio;
                        __prio_list_resort(owner->blocked_on,owner,true);
                    }break;
                    
                    default:
                    {
                        owner->raised_priority = prio;
                    }break;
                }
            }
        }
    }
#endif
}

BAD_RTOS_STATIC bad_rtos_status_t __mutex_delete(bad_mutex_t *mut)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!mut)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!mut->blockedq.next)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else if(kernel_cb.curr != mut->owner)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        kernel_cb.curr->mutex_count--;
        
        if(!kernel_cb.curr->mutex_count)
        {
            kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
        }
        
        __synchro_wake_all(&mut->blockedq,__mutex_timeout_cb,BAD_RTOS_STATUS_DELETED);
        
        *mut = (bad_mutex_t){0};
    }
    
    return ret;
}

#ifdef BAD_RTOS_MUTEX_SIMPLE_PI

static inline bad_rtos_status_t __mutex_do_pi(bad_mutex_t *mut)
{
    if(kernel_cb.curr->raised_priority < mut->owner->raised_priority)
    {
        bad_rtos_status_t ret = __readyq_dequeue(mut->owner); 
        
        mut->owner->raised_priority = kernel_cb.curr->raised_priority;
        
        if(ret == BAD_RTOS_STATUS_OK)
            __readyq_enqueue(mut->owner);
    }
    
    return BAD_RTOS_STATUS_OK; // Intended, ret is not an error
}

#else

static inline bad_rtos_status_t __mutex_do_pi(bad_mutex_t *mut)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    u8 prio = kernel_cb.curr->raised_priority;
    bad_mutex_t *curr_mut = mut;
    
    for(u32 i = 0; i < BAD_RTOS_MUTEX_MAX_PI_ITERS; i++)
    {
        bad_tcb_t *owner = curr_mut->owner;
        
        if(owner == kernel_cb.curr)
        {
            ret = BAD_RTOS_STATUS_DEADLOCK;
            break;
        }
        
        if(owner->raised_priority > prio)
        {
            switch(owner->misc)
            {
                case BAD_RTOS_MISC_READYQ_MEMBER:
                {
                    __readyq_dequeue(owner);
                    owner->raised_priority = prio;
                    __readyq_enqueue(owner);
                }break;
                
                case BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER:
                case BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER:
                {
                    owner->raised_priority = prio;
                    __prio_list_resort(owner->blocked_on,owner,false);
                }break;
                
                default:
                {
                    owner->raised_priority = prio;
                }
            }
        }
        
        if(owner->misc == BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER)
            curr_mut = BAD_CONTAINER_OF(owner->blocked_on,bad_mutex_t,blockedq);
        else
            break;
    }
    
    return ret;
}

#endif

BAD_RTOS_STATIC bad_rtos_status_t __mutex_take(bad_mutex_t *mut, u32 delay)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!mut)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!mut->blockedq.next)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        if(!mut->owner)
        {
            mut->owner = kernel_cb.curr;
            kernel_cb.curr->mutex_count++;
        }
        else if(mut->owner == kernel_cb.curr)
        {
            mut->rec_takes++;
        }
        else
        {
            if (delay != UINT32_MAX && !preempt_count)
            {
                ret = __mutex_do_pi(mut);
            }
            
            if(ret == BAD_RTOS_STATUS_OK)
            {
                ret = __synchro_block(&mut->blockedq,__mutex_timeout_cb,delay, BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER);
            }
        }
    }
    
    return ret;
}   

BAD_RTOS_STATIC bad_rtos_status_t __mutex_put(bad_mutex_t *mut)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!mut)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!mut->blockedq.next)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else if(kernel_cb.curr!= mut->owner)
    {
        ret = BAD_RTOS_STATUS_NOT_OWNER;
    }
    else
    {
        if(mut->rec_takes)
        {
            mut->rec_takes--;
        }
        else
        {
            u32 last_mutex =  !--kernel_cb.curr->mutex_count;
            if(last_mutex)
                kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
            
            mut->owner = __prio_list_dequeue_head(&mut->blockedq); 
            
            if(mut->owner)
            {
                bad_tcb_t *tcb = mut->owner;
                
                tcb->mutex_count++;
                
                if(tcb->cbptr == __mutex_timeout_cb)
                    __delayq_dequeue(mut->owner);
                
                __arm_write_retval(tcb,BAD_RTOS_STATUS_OK);
                
                __readyq_enqueue(mut->owner);
            }
            
            if(last_mutex || mut->owner)
                __sched_try_update();
        }
    }
    
    return ret;
}
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
bad_rtos_status_t sem_init(bad_sem_t *sem, u32 reset_value)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!sem || sem->init_flag)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else
    {
        sem->blockedq = DLIST_INITIALISER(sem->blockedq);
        sem->counter = reset_value;
        sem->init_flag = 1;
    }
    
    return ret;
}

BAD_RTOS_STATIC void __sem_timeout_cb(bad_task_handle_t handle ,void *semaphore)
{
    (void)semaphore;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    dlist_remove(&tcb->qnode);
    
    __arm_write_retval(tcb,BAD_RTOS_STATUS_TIMEOUT);
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_delete(bad_sem_t *sem)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!sem)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!sem->init_flag)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        __synchro_wake_all(&sem->blockedq,__sem_timeout_cb,BAD_RTOS_STATUS_DELETED);
        
        *sem = (bad_sem_t){0};
    }
    
    return ret;
}

bad_rtos_status_t sem_take(bad_sem_t *sem,u32 delay)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!sem)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!sem->init_flag)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        u32 counter = 0;
        
        do
        {
            counter = __ldrex(&sem->counter);
            
            if(!counter)
            {
                __clrex();
                
                if(delay == UINT32_MAX)
                    ret = BAD_RTOS_STATUS_WOULD_BLOCK; 
                else
                    ret = __svc_sem_take(sem,delay);
                
                goto exit;
            }
        }
        while(__strex(counter - 1, &sem->counter));
    }
    
    exit:
    return ret;
}

bad_rtos_status_t sem_put(bad_sem_t *sem)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!sem)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!sem->init_flag)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        u32 counter = 0;
        
        do
        {
            counter = __ldrex(&sem->counter);
            
            if(VOLATILE_READ(sem->blockedq.next) != &sem->blockedq)
            {
                __clrex();
                
                ret = __svc_sem_put(sem);
                goto exit;
            }
        }
        while(__strex(counter + 1, &sem->counter));
    }
    
    exit:
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_put(bad_sem_t *sem)
{
    bad_tcb_t *tcb = __synchro_wake(&sem->blockedq,__sem_timeout_cb,BAD_RTOS_STATUS_OK);
    
    if(!tcb)
    {
        u32 counter = 0;
        
        do
        {
            counter = __ldrex(&sem->counter);
        }
        while(__strex(counter + 1, &sem->counter));
    }
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_take(bad_sem_t *sem, u32 delay)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!sem->counter)
    {
        ret = __synchro_block(&sem->blockedq, __sem_timeout_cb, delay, BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER);
    }
    else
    {
        u32 counter = 0;
        
        do
        {
            counter = __ldrex(&sem->counter);
        }
        while(__strex(counter - 1, &sem->counter));
    }
    
    return ret;
} 

bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!in_isr())
    {
        ret = BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    else if(!sem)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!sem->init_flag)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        u32 counter;
        
        do
        {
            counter = __ldrex(&sem->counter);
            
            if(!counter)
            {
                __clrex();
                
                ret = __kernel_notify(BAD_ISR_OP_SEM_PUT,sem);
                goto exit;
            }
        }
        while(__strex(counter + 1, &sem->counter));
    }
    
    exit:
    return ret;
}
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
static void __event_barrier_timeout_cb(bad_task_handle_t handle ,void *event_barrier)
{
    (void)event_barrier;
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    dlist_remove(&tcb->qnode);
    
    __arm_write_retval(tcb,BAD_RTOS_STATUS_TIMEOUT);
}

bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, u32 count)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!event_barrier|| !count || count >= 32)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(event_barrier->count && event_barrier->count != 32)
    {
        ret = BAD_RTOS_STATUS_IN_USE;
    }
    else
    {
        *event_barrier = (bad_event_barrier_t){0};
        event_barrier->blockedq = DLIST_INITIALISER(event_barrier->blockedq);
        BAD_OPT_BARRIER;
        
        event_barrier->count = count;
    }
    
    return ret;
}

BAD_RTOS_STATIC u32 __event_barrier_wait(bad_event_barrier_t *event_barrier,u32 delay)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!event_barrier)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!event_barrier->count)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else if(event_barrier->count == 32)
    {
        ret = BAD_RTOS_STATUS_FIRED;
    }
    else
    {
        ret = __synchro_block(&event_barrier->blockedq,__event_barrier_timeout_cb,delay, BAD_RTOS_MISC_SYNCHRO_BLOCKEDQ_MEMBER);
    }
    
    return ret; 
}

bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier,u32 flag)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!in_isr())
    {
        ret = BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    else if(!event_barrier ||!flag ||flag == EVENT_BARRIER_FLAGS_VALID_MASK)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!event_barrier->count)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        u32 new_flags = 0;
        u32 flags = 0;
        u32 fired = 0;
        
        do
        {
            flags = __ldrex(&event_barrier->flags);
            
            if(flags & EVENT_BARRIER_FLAGS_VALID_MASK)
            { 
                __clrex(); 
                ret = BAD_RTOS_STATUS_FIRED; 
                goto exit; 
            }
            
            new_flags = flags | flag;
            if(new_flags == flags)
            { 
                __clrex(); 
                goto exit; 
            }
            
            fired = __builtin_popcount(new_flags) >= event_barrier->count;
            
            if(fired)
                new_flags |= EVENT_BARRIER_FLAGS_VALID_MASK;
        }
        while(__strex(new_flags, &event_barrier->flags));
        
        if(fired)
        {
            event_barrier->count = 32;
            ret = __kernel_notify(BAD_ISR_OP_EVENT_BARRIER_WAKE, event_barrier);
        } 
    }
    
    exit:
    return ret;
}

BAD_RTOS_STATIC void __event_barrier_wake(bad_event_barrier_t *event_barrier)
{
    u32 flags = event_barrier->flags;
    
    __synchro_wake_all(&event_barrier->blockedq,__event_barrier_timeout_cb,flags);
}

BAD_RTOS_STATIC bad_rtos_status_t __event_barrier_fire(bad_event_barrier_t *event_barrier,u32 flag)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!event_barrier ||!flag ||flag == EVENT_BARRIER_FLAGS_VALID_MASK)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!event_barrier->count)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED; 
    }
    else
    {
        u32 new_flags = 0;
        u32 flags = 0;
        u32 fired = 0;
        
        do
        {
            flags = __ldrex(&event_barrier->flags);
            
            if(flags & EVENT_BARRIER_FLAGS_VALID_MASK)
            { 
                __clrex(); 
                ret = BAD_RTOS_STATUS_FIRED; 
                goto exit; 
            }
            
            new_flags = flags | flag;
            if(new_flags == flags)
            { 
                __clrex(); 
                goto exit; 
            }
            
            fired = __builtin_popcount(new_flags) >= event_barrier->count;
            
            if(fired)
                new_flags |= EVENT_BARRIER_FLAGS_VALID_MASK;
        }
        while(__strex(new_flags, &event_barrier->flags));
        
        if(fired)
        {
            event_barrier->count = 32;
            __event_barrier_wake(event_barrier);
        }
    }
    
    exit:
    return ret;
}

BAD_RTOS_STATIC bad_rtos_status_t __event_barrier_delete(bad_event_barrier_t *event_barrier)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    
    if(!event_barrier)
    {
        ret = BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    else if(!event_barrier->count)
    {
        ret = BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    else
    {
        __synchro_wake_all(&event_barrier->blockedq,__event_barrier_timeout_cb,BAD_RTOS_STATUS_DELETED);
        
        *event_barrier = (bad_event_barrier_t){0};
    }
    
    return ret;
}
#endif

//ISRS
__attribute__((used))
static void __svc_c(u8 svc, u32* stack)
{
    bad_task_handle_t handle = {0};
    handle.val = stack[0];
    
    switch(svc)
    {
        // Callable only when !preempt_count and kernel is running
        case 0x4:
        {
            __task_finish();
            stack[0] = BAD_RTOS_STATUS_CANT_FINISH;
        }break;
        
        case 0x5:
        {
            stack[0] = __task_yield();
        }break;
        
        case 0x6:
        {
            __task_block();            
            stack[0] = BAD_RTOS_STATUS_WOKEN;
        }break;
        
        case 0x7:
        {
            stack[0] = __task_delay(stack[0], (cbptr) stack[1] ,(void*)stack[2]); 
        }break;
        
        // Callable only when kernel is running
#ifdef BAD_RTOS_USE_SEMAPHORE
        case 0xA:
        {
            stack[0] = __sem_put((bad_sem_t *)stack[0]);
        }break;
        
        case 0xB:
        {
            stack[0] = __sem_take((bad_sem_t*)stack[0] , stack[1]);
        }break;
        
        case 0xC:
        {
            stack[0] = __sem_delete((bad_sem_t*)stack[0]);
        }break;
#endif
        
#ifdef BAD_RTOS_USE_MUTEX
        case 0xD:
        {
            stack[0] = __mutex_put((bad_mutex_t*)stack[0]);
        }break;
        
        case 0xE:
        {
            stack[0] = __mutex_take((bad_mutex_t*)stack[0], stack[1]);
        }break;
        
        case 0xF:
        {
            stack[0] = __mutex_delete((bad_mutex_t*)stack[0]);
        }break;
#endif
        
#ifdef BAD_RTOS_USE_MSGQ
        case 0x10:
        {
            stack[0] = __msgq_post_msg((bad_msgq_t *)stack[0],stack[1],(void*)stack[2],stack[3]);
        }break;
        
        case 0x11:
        {
            stack[0] = __msgq_pull_msg((bad_msgq_t *)stack[0],(bad_msg_block_t *)stack[1],stack[2]);
        }break;
        
        case 0x12:
        {
            stack[0] = __msgq_acquire((bad_msgq_t *)stack[0]);
        }break;
        
        case 0x13:
        {
            stack[0] = __msgq_release((bad_msgq_t *)stack[0]);
        }break;
#ifdef BAD_RTOS_USE_KHEAP
        case 0x14:
        {
            stack[0] = __msgq_acquire_allocate((bad_msgq_t *)stack[0],stack[1]);
        }break;
        
        case 0x15:
        {
            stack[0] = __msgq_release_deallocate((bad_msgq_t *)stack[0]);
        }break;
#endif
#endif
        
#ifdef BAD_RTOS_USE_EVENT_BARRIER
        case 0x16:
        {
            stack[0] = __event_barrier_wait((bad_event_barrier_t *)stack[0],stack[1]);
        }break;
        
        case 0x17:
        {
            stack[0] = __event_barrier_fire((bad_event_barrier_t *)stack[0],stack[1]);
        }break;
        
        case 0x18:
        {
            stack[0] = __event_barrier_delete((bad_event_barrier_t *)stack[0]);
        }break;
#endif
        
        case 0x19:
        {
            stack[0] = __irq_acquire(stack[0]);
        }break;
        
        case 0x20:
        {
            stack[0] = __irq_enable(stack[0]);
        }break;
        
        case 0x21:
        {
            stack[0] = __irq_disable(stack[0]);
        }break;
        
        case 0x22:
        {
            stack[0] = __irq_pend(stack[0]);
        }break;
        
        case 0x23:
        {
            stack[0] = __irq_clear(stack[0]);
        }break;
        
        case 0x24:
        {
            stack[0] = __irq_set_prio(stack[0],stack[1]);
        }break;
        
        case 0x25:
        {
            stack[0] = __irq_release(stack[0]);
        }break;
        
        case 0x26:
        {
            stack[0]= __task_unblock(handle);
        }break;
        
        case 0x27:
        {
            stack[0] = __task_delay_cancel(handle);
        }break;
        
        // Always callable
        case 0xF0:
        {
            __sched_try_update();
        }break;
        
#ifdef BAD_RTOS_USE_KHEAP
        case 0xF2:
        {
            stack[0] = (u32)__kernel_alloc(stack[0]);
        }break;
        
        case 0xF3:
        {
            __kernel_free((void*)stack[0], stack[1]);
        }break;
#endif
        
        case 0xF4:
        {
            stack[0] = __task_make((const bad_task_descr_t*)stack[0]).val;
        }break;
        
        case 0xF5:
        {
            __kernel_start();
        }break;
        
        default:
        {
            __builtin_trap();
        }
    }
}

__attribute__((used))
static void __pendsv_c()
{
    bad_isr_op_obj_t *msg;
    
    while((msg = __isr_q_pop(&kernel_cb.isrq)))
    {
        bad_task_handle_t handle = {.val = (u32)msg->arg};
        
        switch(msg->op_kind)
        {
            case BAD_ISR_OP_TASK_DELAY_CANCEL:
            {
                __task_delay_cancel(handle);
            }break;
            
            case BAD_ISR_OP_TASK_UNBLOCK:
            {
                __task_unblock(handle);
            }break;
            
#ifdef BAD_RTOS_USE_SEMAPHORE
            case BAD_ISR_OP_SEM_PUT:
            {
                __sem_put((bad_sem_t *)(msg->arg));
            }break;
#endif
            
#ifdef BAD_RTOS_USE_MSGQ
            case BAD_ISR_OP_MSGQ_WAKE:
            {
                __msgq_try_wake((bad_msgq_t *)(msg->arg));
            }break;
#endif
            
#ifdef BAD_RTOS_USE_EVENT_BARRIER
            case BAD_ISR_OP_EVENT_BARRIER_WAKE:
            {
                __event_barrier_wake((bad_event_barrier_t *)(msg->arg));
            }break;
#endif
            default:
            {
                __builtin_unreachable();
            }
        }
        gpool_free(msg);
    }
}

__attribute__((used))
static void __handle_systick_event(bad_systick_status_t status)
{
    if(status > 1)
    {
        do
        { 
            bad_tcb_t *wake = __delayq_dequeue_head();
            
            if(wake->cbptr)
            {
                bad_task_handle_t wake_handle = {
                    .idx = __tcb_slab_get_idx_from_ptr(wake),
                    .gen = wake->generation
                };
                
                wake->cbptr(wake_handle,wake->args);
                wake->cbptr = 0;
                wake->args = 0;
            }
            
            wake->counter = wake->ticks_to_change;
            __readyq_enqueue(wake);
        }
        while(!dlist_is_empty(&kernel_cb.delayq) && !BAD_CONTAINER_OF(kernel_cb.delayq.next, bad_tcb_t, delaynode)->counter);
    }
    
    if(status & BAD_SYSTICK_TIMEFRAME_PENDING)
        kernel_cb.curr->counter = kernel_cb.curr->ticks_to_change;
    
    u32 top_ready_prio = __get_top_ready_prio(); 
    
    if(!preempt_count && kernel_cb.ready_bmap && 
       top_ready_prio + (status == BAD_SYSTICK_DELAY_WAKE_PENDING)
       <= kernel_cb.curr->raised_priority)
    {
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}

// ASM stuff
void __attribute__((naked)) BAD_RTOS_SVC_HANDLER_NAME()
{
    __asm__ volatile(
                     "svc_isr:               \n"
                     "tst lr, #4             \n"
                     "ite eq                 \n"
                     "mrseq r1, msp          \n"
                     "mrsne r1, psp          \n"
                     "ldr r3, [r1,#24]       \n"
                     "ldrb r0, [r3,#-2]      \n"
                     "ldr r3,=%0             \n"
                     "ldr r3,[r3]            \n"
                     "cbnz r3,.L_sched_locked\n"
                     ".L_svc_cont:           \n"
                     "push {r7,lr}           \n"
                     BAD_CFI(".cfi_adjust_cfa_offset 8\n"
                             ".cfi_rel_offset r7, 0  \n"
                             ".cfi_rel_offset lr, 4  \n")
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%4            \n"
                     "ldr r3,[r12,#8]        \n"
                     "bic r2,r3,#1           \n"
                     "str r2,[r12,#8]        \n"
                     "push {r3,r12}          \n"
                     BAD_CFI(".cfi_adjust_cfa_offset 8\n"
                             ".cfi_rel_offset r3, 0  \n"
                             ".cfi_rel_offset r12, 4 \n")
#endif
                     "bl __svc_c             \n"
#ifdef BAD_RTOS_USE_MPU
                     "pop {r3,r12}            \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r3         \n"
                             ".cfi_restore r12        \n")
#endif
                     "pop {r7,lr}            \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r7        \n"
                             ".cfi_restore lr        \n")
                     "b __try_context_switch \n"
                     ".L_sched_locked:       \n"//todo : produce correct debug info, this works just because its 0 sum
                     "ldr r3,=%2             \n"
                     "ldrb r3,[r3]           \n"
                     "cmp r3,#1              \n"
                     "ittee ne               \n"
                     "movne r2,0xF0          \n"
                     "movne r12,%3           \n"
                     "moveq r2,0xA           \n"
                     "moveq r12,%1           \n"
                     "cmp r0,r2              \n"
                     "bhs .L_svc_cont        \n"
                     "str r12,[r1]           \n"
                     "bx lr                  \n"
                     ".ltorg                 \n"
                     :
                     :"i"(&preempt_count),"i"(BAD_RTOS_STATUS_SCHED_LOCKED)
                     ,"i"(&kernel_cb.is_running),"i"(BAD_RTOS_STATUS_NOT_RUNNING)
#ifdef BAD_RTOS_USE_MPU
                     ,"i"(&BAD_MPU->RNR)
#endif
                     :
                     );
}

//pendsv isr
void __attribute__((naked)) BAD_RTOS_PENDSV_HANDLER_NAME()
{
    __asm__ volatile(
                     "push {r7,lr}             \n"
                     BAD_CFI(".cfi_adjust_cfa_offset 8 \n"
                             ".cfi_rel_offset r7, 0    \n"
                             ".cfi_rel_offset lr, 4    \n")
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%0              \n"
                     "ldr r3,[r12,#8]          \n"
                     "bic r0,r3,#1             \n"
                     "str r0,[r12,#8]          \n"
                     "push {r3,r12}            \n"
                     BAD_CFI(".cfi_adjust_cfa_offset 8 \n"
                             ".cfi_rel_offset r3, 0    \n"
                             ".cfi_rel_offset r12, 4   \n")
#endif
                     "bl __pendsv_c            \n"
#ifdef BAD_RTOS_USE_MPU
                     "pop {r3,r12}             \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r3          \n"
                             ".cfi_restore r12         \n")
#endif
                     "pop {r7,lr}              \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r7          \n"
                             ".cfi_restore lr          \n")
                     "b __try_context_switch   \n"
                     :
                     :
#ifdef BAD_RTOS_USE_MPU
                     "i"(&BAD_MPU->RNR)
#endif
                     :
                     );
}

void __attribute__((naked)) BAD_RTOS_TICK_HANDLER_NAME()
{
    __asm__ volatile(
                     "ldr r2,=%0               \n"
                     "ldrb r0,[r2,#20]         \n"
                     "cbz r0, exit             \n"
                     "b cont                   \n"
                     "exit:                    \n"
                     "bx lr                    \n"
                     "cont:                    \n"
                     "push {r7,lr}             \n"
                     BAD_CFI(".cfi_adjust_cfa_offset 8 \n"
                             ".cfi_rel_offset r7, 0    \n"
                             ".cfi_rel_offset lr, 4    \n")
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%1              \n"
                     "ldr r3,[r12,#8]          \n"
                     "bic r0,r3,#1             \n"
                     "str r0,[r12,#8]          \n"
                     "push {r3,r12}            \n"
                     BAD_CFI(".cfi_adjust_cfa_offset 8 \n" 
                             ".cfi_rel_offset r3, 0    \n"
                             ".cfi_rel_offset r12, 4   \n")
#endif
                     "ldr r1,[r2]              \n"
                     "adds r1,#1               \n"
                     "str r1,[r2]              \n"
                     "ldr r1,[r2,#4]           \n"
                     "ldr r0,[r1,#44]          \n"
                     "subs r0,#1               \n"
                     "str r0,[r1,#44]          \n"
                     "ite eq                   \n"
                     "moveq r0,#1              \n"
                     "movne r0,#0              \n"
                     "add r1,r2,#12            \n"
                     "ldr r2,[r2,#16]          \n"
                     "cmp r1, r2               \n"
                     "bne .L_nz_delayq         \n"
                     "b .L_skip_delayq         \n"
                     ".L_nz_delayq:            \n"
                     "ldr r1,[r2,#12]          \n"
                     "subs r1,#1               \n"
                     "str r1,[r2,#12]          \n"
                     "it eq                    \n"
                     "orreq r0,#2              \n"
                     ".L_skip_delayq:          \n"
                     "cbnz r0,.L_handle_event  \n"
                     BAD_CFI(".cfi_remember_state      \n")
#ifdef BAD_RTOS_USE_MPU
                     "pop {r3,r12}             \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r3          \n"
                             ".cfi_restore r12         \n")
                     "str r3,[r12,#8]          \n"
                     "dsb                      \n"
#endif
                     "pop {r7,pc}              \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r7          \n"
                             ".cfi_restore pc          \n")
                     
                     ".L_handle_event:         \n"
                     BAD_CFI(".cfi_restore_state       \n")
                     "bl __handle_systick_event\n"
                     
                     "pop {r3,r12}             \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r3          \n"
                             ".cfi_restore r12         \n")
                     
                     "pop {r7,lr}              \n"
                     BAD_CFI(".cfi_adjust_cfa_offset -8\n"
                             ".cfi_restore r7          \n"
                             ".cfi_restore lr          \n")
                     
                     "b __try_context_switch   \n"
                     ".ltorg                   \n"
                     :
                     : "i" (&kernel_cb)
#ifdef BAD_RTOS_USE_MPU
                     ,"i" (&BAD_MPU->RNR)
#endif
                     :
                     );
}

//Common context switch code
static void __attribute__((naked,used)) __try_context_switch()
{
    __asm__ volatile(
                     "ldr r1,=kernel_cb        \n"
                     "ldr r2,[r1,#8]           \n"
                     "cbnz r2,.L_context_switch\n"
#ifdef BAD_RTOS_USE_MPU
                     "str r3,[r12,#8]          \n"
                     "dsb                      \n"
#endif
                     "bx lr                    \n"
                     ".L_context_switch:       \n"
                     "mrs r0,psp               \n"
#ifdef BAD_RTOS_USE_FPU
                     "tst lr,#0x10             \n"
                     "it eq                    \n"
                     "vstmdbeq r0!, {s16-s31}  \n"
#endif
                     "stmdb r0!, {r4-r11,lr}   \n"
                     "ldr r4,[r1,#4]           \n"
                     "str r0,[r4]              \n"
                     "ldr r0,[r2]              \n"
                     "str r2,[r1,#4]           \n"
                     "movs r4,#0               \n"
                     "str r4,[r1,#8]           \n"
                     BAD_RTOS_ASM_LOAD_PSPLIM
#ifdef BAD_RTOS_USE_MPU
                     BAD_RTOS_ASM_SET_RNR
                     "adds r2,#48              \n"
                     "add r1,r12,#4            \n"
                     "ldmia r2!,{r4-r11}       \n"
                     "stmia r1!,{r4-r11}       \n"
                     "mov r2,#7                \n"
                     "str r2,[r12]             \n"
                     "str r3,[r12,#8]          \n"
                     "dsb                      \n"
                     "isb                      \n"
#endif
                     "ldmia r0!, {r4-r11,lr}   \n"
#ifdef BAD_RTOS_USE_FPU
                     "tst lr,#0x10             \n"
                     "it eq                    \n"
                     "vldmiaeq r0!, {s16-s31}  \n"
#endif
                     "msr psp,r0               \n"
                     "bx lr                    \n"
                     ".ltorg                   \n"
                     );
}

//first task start
void __attribute__((naked)) __init_second_stage()
{
    __asm__ volatile(
                     "ldr r1,=__estack           \n"
                     "msr msp,r1                 \n"
                     "ldr r1,=kernel_cb          \n"
                     "ldr r2,[r1,#4]             \n"
                     "ldr r0,[r2]                \n"
                     BAD_RTOS_ASM_LOAD_PSPLIM
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%0                \n"
                     BAD_RTOS_ASM_SET_RNR
                     "adds r2, #48               \n"
                     "add r1,r12,#4              \n"
                     "ldmia r2!,{r4-r11}         \n"
                     "stmia r1!,{r4-r11}         \n"
                     "mov r2,#7                  \n"
                     "str r2,[r12]               \n"
                     "ldr r3,[r12,#8]            \n"
                     "orrs r3,#1                 \n"
                     "str r3,[r12,#8]            \n"
                     "dsb                        \n"
                     "isb                        \n"
#endif
                     "ldmia r0!,{r4-r11,lr}      \n"
                     "msr psp, r0                \n"
                     "bx lr                      \n"
                     ".ltorg                     \n"
                     :
                     :
#ifdef BAD_RTOS_USE_MPU
                     "i"(&BAD_MPU->RNR)          
#endif
                     :
                     );
}

//idle task
__asm__(
        ".thumb_func                    \n"
        ".global idle_task              \n"
        "idle_task:                     \n"
        "infinite_loop:                 \n"
        "dsb                            \n"
        "wfi                            \n"
        "b infinite_loop                \n"
        );

//SVC calls
__asm__(
        ".thumb_func                    \n"
        ".global task_finish            \n"
        "task_finish:                   \n"
        "svc 0x4                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_yield             \n"
        "task_yield:                    \n"
        "svc 0x5                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_block             \n"
        "task_block:                    \n"
        "svc 0x6                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_delay             \n"
        "task_delay:                    \n"
        "svc 0x7                        \n"
        "bx lr                          \n"
        );


#ifdef BAD_RTOS_USE_SEMAPHORE
__asm__(
        ".thumb_func                    \n"
        ".global __svc_sem_put          \n"
        "__svc_sem_put:                 \n"
        "svc 0xA                        \n"
        "bx lr                          \n"
        );
__asm__(
        ".thumb_func                    \n"
        ".global __svc_sem_take         \n"
        "__svc_sem_take:                \n"
        "svc 0xB                        \n"
        "bx lr                          \n"
        );
__asm__(
        ".thumb_func                    \n"
        ".global sem_delete             \n"
        "sem_delete:                    \n"
        "svc 0xC                        \n"
        "bx lr                          \n"
        );
#endif

#ifdef BAD_RTOS_USE_MUTEX
__asm__(
        ".thumb_func                    \n"
        ".global mutex_put              \n"
        "mutex_put:                     \n"
        "svc 0xD                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global mutex_take             \n"
        "mutex_take:                    \n"
        "svc 0xE                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global mutex_delete           \n"
        "mutex_delete:                  \n"
        "svc 0xF                        \n"
        "bx lr                          \n"
        );

#endif

#ifdef BAD_RTOS_USE_MSGQ
__asm__(
        ".thumb_func                    \n"
        ".global msgq_post_msg          \n"
        "msgq_post_msg:                 \n"
        "svc 0x10                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global msgq_pull_msg          \n"
        "msgq_pull_msg:                 \n"
        "svc 0x11                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global msgq_acquire           \n"
        "msgq_acquire:                  \n"
        "svc 0x12                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global msgq_release           \n"
        "msgq_release:                  \n"
        "svc 0x13                       \n"
        "bx lr                          \n"
        );

#ifdef BAD_RTOS_USE_KHEAP
__asm__(
        ".thumb_func                    \n"
        ".global msgq_acquire_allocate  \n"
        "msgq_acquire_allocate:         \n"
        "svc 0x14                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global msgq_release_deallocate\n"
        "msgq_release_deallocate:       \n"
        "svc 0x15                       \n"
        "bx lr                          \n"
        );
#endif
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
__asm__(
        ".thumb_func                    \n"
        ".global event_barrier_wait     \n"
        "event_barrier_wait:            \n"
        "svc 0x16                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global event_barrier_fire     \n"
        "event_barrier_fire:            \n"
        "svc 0x17                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global event_barrier_delete   \n"
        "event_barrier_delete:          \n"
        "svc 0x18                       \n"
        "bx lr                          \n"
        );
#endif

__asm__(
        ".thumb_func                    \n"
        ".global irq_acquire            \n"
        "irq_acquire:                   \n"
        "svc 0x19                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global irq_enable             \n"
        "irq_enable:                    \n"
        "svc 0x20                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global irq_disable            \n"
        "irq_disable:                   \n"
        "svc 0x21                       \n" 
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global irq_pend               \n"
        "irq_pend:                      \n"
        "svc 0x22                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global irq_clear              \n"
        "irq_clear:                     \n"
        "svc 0x23                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global irq_set_prio           \n"
        "irq_set_prio:                  \n"
        "svc 0x24                       \n" 
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global irq_release            \n"
        "irq_release:                   \n"
        "svc 0x25                       \n" 
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_unblock           \n"
        "task_unblock:                  \n"
        "svc 0x26                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_delay_cancel      \n"
        "task_delay_cancel:             \n"
        "svc 0x27                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global __svc_check_resched    \n"
        "__svc_check_resched:           \n"
        "svc 0xF0                       \n" 
        "bx lr                          \n"
        );

#ifdef BAD_RTOS_USE_KHEAP
__asm__(
        ".thumb_func                    \n"
        ".global kernel_alloc           \n"
        "kernel_alloc:                  \n"
        "svc 0xF2                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global kernel_free            \n"
        "kernel_free:                   \n"
        "svc 0xF3                       \n"
        "bx lr                          \n"
        );
#endif

__asm__(
        ".thumb_func                    \n"
        ".global task_make              \n"
        "task_make:                     \n"
        "svc 0xF4                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global __first_task_start     \n"
        "__first_task_start:            \n"
        "svc 0xF5                       \n" 
        "bx lr                          \n"
        );


//helpers for specific common operations
static inline __attribute__((always_inline)) u32 __modify_basepri(u32 new_basepri)
{
    u32 old_basepri;
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

static inline __attribute__((always_inline)) void __restore_basepri(u32 new_basepri)
{
    __asm__ volatile(
                     "msr basepri, %0    \n"
                     "isb                \n"
                     : : "r"(new_basepri)
                     : "memory"
                     );
}

static inline __attribute__((always_inline)) void __set_control(u32 new_control)
{
    __asm__ volatile(
                     "msr control, %0    \n"
                     "isb                \n"
                     : : "r"(new_control)
                     : "memory"
                     ); 
}

static inline __attribute__((always_inline)) u32 __get_control()
{
    u32 res;
    __asm__ volatile("mrs %0, control":"=r"(res));
    return res;
}

#endif

#endif
