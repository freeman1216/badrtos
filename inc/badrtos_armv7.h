/**
* @file badrtos_armv7.h
* @brief Header only rtos implementation
*
*
*
* Usage:
*  - Include this file and define BAD_RTOS_IMPLEMENTATION in **one**
*    C file
*  - Change the config to your liking
*  - Define the bad_user_init function and all the perliminary setup there, like task creation 
*  - Call bad_rtos_start to start rtos operation
* Notes:
*  - Depends on the linker file , to port just edit the linker file adding nessesary sections at the beginning of ram :
*     .kernel_bss (NOLOAD) : ALIGN(32)
*    {
*         __kernel_bss = .;
*         *(.kernel_bss)
*         __ekernel_bss = .;
*    
*     } > RAM
*
*     __rkernel_data = LOADADDR(.kernel_data);
*
*	  .kernel_data : ALIGN(4) 
*	 {
*		 __kernel_data = .;
*		     *(.kernel_data)
*		 __ekernel_data = .;
*	 } > RAM AT > ROM
*
*	  .static_stacks : ALIGN(4096)
*	 {
*		 __static_stacks = .;
*         *(.static_stacks)
*         __estatic_stacks = .;
*	 }
*     .heap : ALIGN(32)
*     {
*         __heap = .;
*         *(.kheap)
*     } > RAM
*
*  - ! Kernel syscall interrupt priority is 0 on startup ,
*      after startup it drops to lowest alowing isrs to run freely, 
*      all the interaction between the kernel and isrs are done through pendsv triggering functions
*      
*  - !! If the task uses FPU make sure the stack size can accomodate additional 33 registers 

// PUBLIC API**********************************************

**
* \b TASK_HANDLE_IS_VALID
*  Public macro to check the validity of the task handle
*  
*  @param[in] bad_task_handle_t task handle
*
*  @retval 1 valid
*  @retval 0 invalid
*
#define TASK_HANDLE_IS_VALID(handle)

**
* \b BAD_TASK_HANDLE_INVALID_GET_ERROR
*  Public macro to get error from invalid taskhandle
*  
*  @param[in] bad_task_handle_t invalid task handle
*
*  @retval BAD_RTOS_STATUS_BAD_PARAMETERS on bad configurations
*  @retval BAD_RTOS_STATUS_ALLOC_FAIL on allocation falure
*
*
#define TASK_HANDLE_INVALID_GET_ERROR(handle)

**
* \b task_make
*
* Public SVC (svc 0xF5) call that calls internal function __task_make
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
* extern bad_task_handle_t task_make(bad_task_descr_t *descr);

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
* @param[in] uint32_t delay in ticks 
* @param[in] cbptr cb callback to run 
* @param[in] void* args arguments for the callback
*
* @retval BAD_RTOS_STATUS_OK delay time ran out
* @retval BAD_RTOS_STATUS_WOKEN the task was woken by another task or isr
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t task_delay(uint32_t delay, cbptr cb, void *args );

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
* @retval BAD_RTOS_STATUS_OK task is successfully blocked
* @retval BAD_RTOS_WRONG_CONTEXT the function was called by an isr 
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t task_block();

**
* \b task_unblock
*
* Public SVC (svc 0x2) call that calls internal function __task_unblock
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
* @retval BAD_RTOS_STATUS_OK task is successfully unblocked
* @retval BAD_RTOS_STATUS_NOT_BLOCKED the task is not blocked
* @retval BAD_RTOS_STATUS_HANDLE_INVALID handle is invalid
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
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
* @retval BAD_RTOS_WRONG_CONTEXT if called from thread context
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
* @retval BAD_RTOS_STATUS_CANT_YEILD no task to yield to
* @retval BAD_RTOS_WRONG_CONTEXT the function was called by an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
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
* @retval BAD_RTOS_WRONG_CONTEXT the function was called by an isr 
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t task_finish();

**
* \b task_delay_cancel
*
* Public SVC (svc 0x3) call that calls internal function __task_delay_cancel
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
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
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
* @retval BAD_RTOS_WRONG_CONTEXT if called from thread context
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message 
*
* extern bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t task);

**
* \b sched_lock 
*
* Public svc call (svc 0xF0) that calls internal function __sched_lock
* Disables scheduler operation, stops context switching
* Most of the api is unavailible in this state 
*
* @retval uint32_t previous lock state
*
* extern uint32_t sched_lock();

**
* \b sched_unlock 
*
* Public svc call (svc 0xF1) that calls internal function __sched_unlock
* Enables scheduler operation, restarts context switching
*
* @param[in] uint32_t previous lock state
*
* extern void sched_unlock(uint32_t key);

**
* \b pool_init
*
* Public function 
* Tries to allocate an object from specifed pool allocator
* If a freed block exsists atomically pulls it from the freelist, otherwise lazily allocates it 
* from an assosiated block of memory
*
* This function can be called from interrupt context. This function is reentrant 
* @param[in] bad_pool_t pool to allocate from 
* 
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
*
* extern bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, uint32_t block_size, uint32_t size_in_bytes);

**
* \b pool_init
*
* Public function 
* Tries to allocate an object from specifed pool allocator
* If a freed block exsists atomically pulls it from the freelist, otherwise lazily allocates it 
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
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
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
* \b pool_free
*
* Public function 
* Specialised pool_free function that operates on kernel provided global pool
* 
* This function can be called from interrupt context. This function is reentrant
* 
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
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
* @param[in] uint32_t size in bytes 
* 
* @retval void * to allocated memory
* @retval Null ptr allocation failed 
*
* extern void* kernel_alloc(uint32_t size);

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
* @param[in] uint32_t size in bytes 
* 
*
* extern void kernel_free(void *block,uint32_t size);

// Priority inheriting mutex api
**
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
*
* extern bad_rtos_status_t mutex_init(bad_mutex_t *mut);

**
* \b mutex_take
*
* Public SVC (svc 0xA) call that calls internal function __mutex_take
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
* This function cannot be called from interrupt context.Will generate a fault if done so
*
* @param[in] bad_mutex_t* Ptr to mutex object to try take  
* @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Mutex successfully taken
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
* @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT function was called from an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,uint32_t delay);

**
* \b mutex_put
*
* Public SVC (svc 0xB) call that calls internal function __mutex_put
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
* This function cannot be called from interrupt context.Will generate a fault if done so
*
* @param[in] bad_mutex_t* Ptr to mutex object to try put  
*
* @retval BAD_RTOS_STATUS_OK Mutex successfully put
* @retval BAD_RTOS_STATUS_NOT_OWNER caller is not the owner of this mutex object
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex object is NULL
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t mutex_put(bad_mutex_t *mut);

**
* \b mutex_delete
*
* Public SVC (svc 0xC) call that calls internal function __mutex_delete
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
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t mutex_delete(bad_mutex_t *mut);

// Blocking semaphore api 
**
* \b sem_init
*
* Public function to initialise semaphore object
* initialises count field to the specifed count
*
* No need to call this if you can use an initiliser like bad_sem_t sem = {.counter = N,.init_flag = 1 }
*
* This function can be called from interrupt context. But is not reentrant if the object parameter is the same
* @param[in] bad_sem_t* Ptr to semaphore object to initialise
* @param[in] uint16_t Value to initialise semaphore counter with
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore ptr is null 
*
* extern bad_rtos_status_t sem_init(bad_sem_t *sem,uint32_t reset_value);

**
* \b sem_take
*
* Public SVC (svc 0xD) call that calls internal function __sem_take
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
* This function cannot be called from interrupt context. Will generate a fault if done so
*
* @param[in] bad_sem_t* Ptr to semaphore object to try take  
* @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Semaphore successfully taken
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t sem_take(bad_sem_t *sem,uint32_t delay);

**
* \b sem_put
*
* Public SVC (svc 0xE) call that calls internal function __sem_put
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
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
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
* @retval BAD_RTOS_WRONG_CONTEXT if called from thread context
* @retval BAD_RTOS_ALLOC_FAIL failed to allocate kernel message object
*
* extern bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem);

**
* \b sem_delete
*
* Public SVC (svc 0xF) call that calls internal function __sem_delete
* Tries to delete the semaphore object, doesnt infuence the underlying memory, just resets the object
*
* Wakes up all the tasks with BAD_RTOS_STATUS_DELETED written into their 
* stacked registers 
*
* This function can be called from interrupt context. But loops over semaphores blocked queue
* @param[in] bad_sem_t* Ptr to semaphore object to try delete  
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully deleted
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore object is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t sem_delete(bad_sem_t *sem);

//Message queues
//Macro for static queue allocation
#define MSGQ_STATIC_INIT(name,size)

//Heap dependant api
**
* \b msgq_acquire_allocate
*
* Public SVC call (svc 0x14) that calls internal function __msgq_acquire_allocate.
* Dynamically binds a message queue to the currently running task and allocates kernel memory for its buffer.
*
* The current task becomes the exclusive owner of this message queue (receivers must be owners).
* The capacity must be a power of 2. A task can only own one message queue at a time.
*
* @param[in] bad_msgq_t* q Ptr to message queue object to initialize and bind
* @param[in] uint32_t capacity Number of messages the queue can hold (MUST be a power of 2)
*
* @retval BAD_RTOS_STATUS_OK Queue successfully allocated and bound to current task
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL or capacity is not a power of 2
* @retval BAD_RTOS_STATUS_NOT_OWNER Queue is already owned by another task
* @retval BAD_RTOS_STATUS_ALREADY_BOUND The current task already owns a message queue
*
* extern bad_rtos_status_t msgq_acquire_allocate(bad_msgq_t *q, uint32_t capacity);

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
*
* extern bad_rtos_status_t msgq_release_deallocate(bad_msgq_t *q);

//Heap independant api
**
* \b msgq_acquire
*
* Public SVC call (svc 0x12) that calls internal function __msgq_acquire.
* Statically binds a message queue to the currently running task without allocating memory.
*
* Assumes the message queue buffer has already been statically provisioned.
* The current task becomes the exclusive owner of this message queue.
*
* @param[in] bad_msgq_t* q Ptr to static message queue object to bind
*
* @retval BAD_RTOS_STATUS_OK Queue successfully bound to current task
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_OWNER Queue is already owned by another task
* @retval BAD_RTOS_STATUS_ALREADY_BOUND The current task already owns a message queue
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
*
* extern bad_rtos_status_t msgq_release(bad_msgq_t *q);

**
* \b msgq_pull_msg
*
* Public SVC call (svc 0x10) that calls internal function __msgq_pull_msg.
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
* @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Message successfully pulled
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q or writeback ptr is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_NOT_OWNER Current task is not the owner of this queue
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay is -1 and queue is empty
* @retval BAD_RTOS_STATUS_TIMEOUT blocked for N ticks without receiving a message
*
* extern bad_rtos_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback, uint32_t delay);

**
* \b msgq_post_msg
*
* Public SVC call (svc 0x11) that calls internal function __msgq_post_msg.
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
* @param[in] uint32_t signal The 32-bit signalID of the message
* @param[in] void* args Ptr to message arguments or payload
* @param[in] uint32_t delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Message successfully posted
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay is -1 and queue is full
* @retval BAD_RTOS_STATUS_TIMEOUT blocked for N ticks without space freeing up
*
* extern bad_rtos_status_t msgq_post_msg(bad_msgq_t *q, uint32_t signal, void *args, uint32_t delay);

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
* @param[in] uint32_t signal The 32-bit signal/ID of the message
* @param[in] void* args Ptr to message arguments or payload
*
* @retval BAD_RTOS_STATUS_OK Message successfully posted
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT Called from thread context instead of ISR
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK Queue is full, cannot post
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message
*
* extern bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, uint32_t signal, void *args);
 
//Event barrier 
**
* \b BAD_EVENT_BARRIER_FLAGS_ARE_VALID
*  Public macro to check the validity of the returned flags
*  
*  @param[in] bad_task_handle_t task handle
*
*  @retval 1 valid
*  @retval 0 invalid
*
#define EVENT_BARRIER_FLAGS_VALID_MASK (0x80000000UL)
#define EVENT_BARRIER_FLAGS_ARE_VALID(flags) (!!((flags) & EVENT_BARRIER_FLAGS_VALID_MASK))

**
* \b event_barrier_prime
*
* Public function that primes an event barrier for a new synchronization cycle.
* Initializes the barrier to wait for a specific number of distinct event flags.
* * The count specifies how many distinct flags must be set before the barrier fires. 
* The maximum number of flags is 31.
*
* @param[in] bad_event_barrier_t* Ptr to event barrier object to prime
* @param[in] uint32_t count Number of distinct events required to fire the barrier (1 to 31)
*
* @retval BAD_RTOS_STATUS_OK Event barrier successfully primed
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier ptr is null, count is 0, or count >= 32
* @retval BAD_RTOS_STATUS_IN_USE barrier is currently active/primed and hasn't fired yet
*
* extern bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, uint32_t count);

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
* @param[in] uint32_t delay ticks 0 = block, N = block for N ticks
*
* @retval uint32_t flags | (1 << 31)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier ptr is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired (count == 32)
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay = -1 and barrier has not fired yet 
*
* extern uint32_t event_barrier_wait(bad_event_barrier_t *event_barrier, uint32_t delay);

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
* @param[in] uint32_t flag Bitmask representing the specific event(s) to set
*
* @retval BAD_RTOS_STATUS_OK Flag successfully set (barrier may or may not have fired)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier is NULL, flag is 0, or flag contains invalid bits
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT if called from thread context instead of ISR
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired
* @retval BAD_RTOS_STATUS_ALLOC_FAIL failed to allocate kernel message
*
* extern bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier, uint32_t flag);

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
* @param[in] uint32_t flag Bitmask representing the specific event(s) to set
*
* @retval BAD_RTOS_STATUS_OK Flag successfully set (barrier may or may not have fired)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier is NULL, flag is 0, or flag contains invalid bits
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired
*
* extern bad_rtos_status_t event_barrier_fire(bad_event_barrier_t *event_barrier, uint32_t flag);

**
* \b event_barrier_delete
*
* Public svc call (svc 0x18) that calls internal function __event_barrier_fire
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
*
* extern bad_rtos_status_t event_barrier_delete(bad_event_barrier_t *event_barrier);

//Mpu Macros
**
* /b START_TASK_MPU_REGIONS_DEFINITIONS
* #define START_TASK_MPU_REGIONS_DEFINITIONS(name)

**
* /b DEFINE_GENERIC_REGION
* #define DEFINE_GENERIC_REGION(name,address, size, tex_scb, ap)

**
* /b DEFINE_PERIPH_ACCESS_REGION
* #define DEFINE_PERIPH_ACCESS_REGION(name,address, size) 

**
* /b END_TASK_MPU_REGIONS
* #define END_TASK_MPU_REGIONS(name)

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

#pragma once
#ifndef BAD_RTOS_CORE
#define BAD_RTOS_CORE

#include <stdint.h>

//CONFIG
//uncoment those to enable desired functionality
#define BAD_RTOS_USE_KHEAP      //kernel heap
//#define KMIN_ORDER 5          //kernel heap minimal order of allocation (size = 1 << MIN_ORDER = 32)
//#define KMAX_ORDER 12         //kernel heap maximum order of allocation (heap_size) (size = 1 << MIN_ORDER = 4096)
#define BAD_RTOS_USE_EVENT_BARRIER
#define BAD_RTOS_USE_MUTEX      //mutexes
#define BAD_RTOS_USE_MSGQ       // message queues
#define BAD_RTOS_USE_SEMAPHORE  //semaphores
#define BAD_RTOS_USE_MPU        //mpu
#define BAD_RTOS_USE_FPU        //fpu
#define BAD_RTOS_FPU_DEFAULT_SETTINGS //use default settings for the fpu (lazy + auto stacking enabled),if custom settings used - comment this and enable lazy stacking

#define BAD_RTOS_FLASH_SIZE         (0x80000)
#define BAD_RTOS_GLOBAL_POOL_SIZE   (128)
#define BAD_RTOS_MAX_TASKS          (32)   //maximum number of running tasks, number of user priorities = BAD_RTOS_MAX_TASKS-2, with idle task running at BAD_RTOS_MAX_TASKS-1
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

// error codes 
typedef enum {
    BAD_RTOS_STATUS_ALLOC_FAIL = 0, // to return as null ptr
    BAD_RTOS_STATUS_OK,
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
    BAD_RTOS_STATUS_ALREADY_BOUND,
    BAD_RTOS_STATUS_SCHED_LOCKED,
    BAD_RTOS_STATUS_FIRED,
    BAD_RTOS_STATUS_IN_USE
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
    BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_EVENT_BARRIER_BLOCKEDQ_MEMBER
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
    uint32_t addr;
    uint32_t rasr;
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
    uint8_t *stack;
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
#ifdef BAD_RTOS_USE_MSGQ
    uint8_t msgq_owner;
#endif
}bad_tcb_t;

typedef struct {
    uint8_t * volatile next;
    uint8_t *mem;
    volatile uint32_t curr;
    uint32_t size_in_bytes;
    uint32_t block_size;
}bad_pool_t;

#ifdef BAD_RTOS_USE_MSGQ
typedef struct bad_msg_block{
    uint32_t signal;
    void *args;
} bad_msg_block_t;

typedef struct {
    bad_link_node_t blockedq;
    bad_tcb_t* owner;
    uint16_t capacity;
    uint16_t pad;
    volatile uint16_t head;
    volatile uint16_t tail;
    bad_msg_block_t *msgs;
    uint8_t dynamic;
} bad_msgq_t;
#endif

//task decription for task creation
typedef struct{
    uint32_t stack_size;
    uint8_t* stack;
    taskptr entry;
    void* args;
    uint32_t ticks_to_change;
#ifdef BAD_RTOS_USE_MPU
    const mpu_region_t *regions;
#endif
#ifdef BAD_RTOS_USE_MSGQ
    bad_msgq_t *assigned_msgq;
#endif
    uint8_t base_priority;
}bad_task_descr_t;

#ifdef BAD_RTOS_USE_MUTEX
typedef struct bad_mutex{
    bad_link_node_t blockedq;
    bad_tcb_t *owner;
    uint32_t rec_takes;
} bad_mutex_t ;
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
typedef struct bad_sem{
    bad_link_node_t blockedq;
    volatile uint32_t counter;
    volatile uint32_t init_flag;
} bad_sem_t;
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
typedef struct{
    bad_link_node_t blockedq;
    volatile uint32_t flags;
    volatile uint32_t count;
}bad_event_barrier_t;
#endif

#ifdef BAD_RTOS_USE_MPU

typedef enum{
    BAD_MPU_TEXSCB_STRONGLY_ORDERED = 0,
    BAD_MPU_TEXSCB_SHARED_DEVICE = 0x10000,
    BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRT = 0x20000,
    BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRT_SHAREABLE = 0x60000,
    BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB = 0x30000,
    BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE = 0x70000,
    BAD_MPU_TEXSCB_NORMAL_NON_CACHEABLE = 0x80000,
    BAD_MPU_TEXSCB_NORMAL_NON_CACHEABLE_SHAREABLE = 0xC0000,
    BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE = 0xB0000,
    BAD_MPU_TEXSCB_NORMAL_RW_ALLOCATE_SHAREABLE = 0xF0000,
    BAD_MPU_TEXSCB_NON_SHAREABLE_DEVICE = 0x100000
}bad_mpu_texscb_features_t;

typedef enum{
    BAD_MPU_AP_NO_ACCESS = 0,
    BAD_MPU_AP_PRIV_RW_UNPRIV_FAULT = 0x1000000,
    BAD_MPU_AP_PRIV_RW_UNPRIV_RO = 0x2000000,
    BAD_MPU_AP_FULL_ACCESS = 0x3000000,
    BAD_MPU_AP_PRIV_RO_UNPRIV_FAULT = 0x5000000,
    BAD_MPU_AP_PRIV_RO_UNPRIV_RO = 0x6000000
}bad_mpu_permissions_t;

#define BAD_MPU_NEXT_POW2(x) ( \
(x) <= 32 ? 32 :   \
(x) <= 64 ? 64 :   \
(x) <=128 ? 128:   \
(x) <=256 ? 256:   \
(x) <=512 ? 512:   \
(x) <=1024 ? 1024: \
(x) <=2048 ? 2048: \
(x) <=4096 ? 4096: \
(x) <=8192 ? 8912: \
16384\
)

#define BAD_MPU_PREV_POW2(x)( \
((x) >= 32 && (x) < 64)  ? 32 :     \
((x) >= 64 && (x) < 128) ? 64 :     \
((x) >= 128 && (x) < 256) ? 128 :   \
((x) >= 256 && (x) < 512) ? 256 :   \
((x) >= 512 && (x) < 1024) ? 512 :  \
((x) >= 1024 && (x) < 2048) ? 1024 :\
((x) >= 2048 && (x) < 4096) ? 2048 :\
4096\
)

#define BAD_MPU_FIND_SIZE(x)(\
(x) == 32 ? (4<<1):   \
(x) == 64 ? (5<<1):   \
(x) == 128 ? (6<<1):  \
(x) == 256 ? (7<<1):  \
(x) == 512 ? (8<<1):  \
(x) == 1024 ? (9<<1): \
(x) == 2048 ? (10<<1):\
(x) == 4096 ? (11<<1):\
(x) == 8192 ? (12<<1):\
(x) == 16384 ? (13<<1):\
(14 << 1)\
)

#define BAD_MPU_RBAR_VALID          (1u << 4)
#define BAD_MPU_RBAR_REGION(n)      ((n) & 0x0Fu)
#define BAD_MPU_RBAR_ADDR_MASK      (~0x1Fu)

#define BAD_MPU_RASR_ENABLE         (0x1)
#define BAD_MPU_RASR_XN             (0x10000000)

#define START_TASK_MPU_REGIONS_DEFINITIONS(name)\
enum {name##_COUNTER_BASE = __COUNTER__ }; \
static const mpu_region_t __attribute__((section(".kernel_data"))) name##_regions[3] = {

// We embed the Region Index (1, 2, or 3) directly into the address word
#define DEFINE_GENERIC_REGION(name,address, size, tex_scb, ap)\
[3 - ((__COUNTER__ - name##_COUNTER_BASE + 1) / 2)] = {\
.addr = (uint32_t)(address) | BAD_MPU_RBAR_VALID | BAD_MPU_RBAR_REGION(((__COUNTER__ - name##_COUNTER_BASE + 1) / 2)), \
.rasr = (uint32_t)(ap) | (tex_scb) | BAD_MPU_FIND_SIZE(BAD_MPU_NEXT_POW2(size)) | 0x1 \
},

#define DEFINE_PERIPH_ACCESS_REGION(name,address, size) \
[3 - ((__COUNTER__ - name##_COUNTER_BASE + 1) / 2)] = {\
.addr = (uint32_t)(address) | BAD_MPU_RBAR_VALID | BAD_MPU_RBAR_REGION(((__COUNTER__ - name##_COUNTER_BASE) / 2)), \
.rasr = BAD_MPU_RASR_XN | BAD_MPU_AP_FULL_ACCESS | BAD_MPU_TEXSCB_SHARED_DEVICE | BAD_MPU_FIND_SIZE(BAD_MPU_NEXT_POW2(size)) | 0x1 \
},

#define END_TASK_MPU_REGIONS(name) \
};\

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
#define TASK_HANDLE_IS_VALID(handle) ({ \
_Static_assert(__builtin_types_compatible_p(typeof(handle), (bad_task_handle_t){0}), "Type of variable differs from bad_task_handle_t");\
(((handle) & 0xFFFF) != 0xFFFF);\
})
#define TASK_HANDLE_INVALID_GET_ERROR(handle) ((handle) >> 16)

extern bad_task_handle_t task_make(bad_task_descr_t *descr);
extern bad_rtos_status_t task_delay(uint32_t delay, cbptr cb, void *args);
extern bad_rtos_status_t task_block();
extern bad_rtos_status_t task_unblock(bad_task_handle_t task);
extern bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t task);
extern bad_rtos_status_t task_yield();
extern bad_rtos_status_t task_finish();
extern bad_rtos_status_t task_delay_cancel(bad_task_handle_t task);
extern bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t task);
extern uint32_t sched_lock();
extern void sched_unlock(uint32_t key);
extern bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, uint32_t block_size, uint32_t size_in_bytes);
extern void* pool_alloc(bad_pool_t *pool);
extern void pool_free(bad_pool_t *pool, void *obj);
extern void* gpool_alloc();
extern void gpool_free(void *obj);

#ifdef BAD_RTOS_USE_KHEAP
extern void* kernel_alloc(uint32_t size);
extern void kernel_free(void *block,uint32_t size);
#endif

#ifdef BAD_RTOS_USE_MUTEX
extern bad_rtos_status_t mutex_init(bad_mutex_t *mut);
extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,uint32_t delay);
extern bad_rtos_status_t mutex_put(bad_mutex_t *mut);
extern bad_rtos_status_t mutex_delete(bad_mutex_t *mut);
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
extern bad_rtos_status_t sem_init(bad_sem_t *sem,uint32_t reset_value);
extern bad_rtos_status_t sem_take(bad_sem_t *sem,uint32_t delay);
extern bad_rtos_status_t sem_put(bad_sem_t *sem);
extern bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem);
extern bad_rtos_status_t sem_delete(bad_sem_t *sem);
#endif

#ifdef BAD_RTOS_USE_MSGQ
//Macro for static queue allocation
#define MSGQ_STATIC_INIT(name,size)\
_Static_assert((size & (size-1)) == 0, "queue size must be a power of 2"); \
bad_msg_block_t name##_blocks [size];\
bad_msgq_t name = {.capacity = size,.msgs = name##_blocks};
#ifdef BAD_RTOS_USE_KHEAP
extern bad_rtos_status_t msgq_acquire_allocate(bad_msgq_t *q, uint16_t capacity);
extern bad_rtos_status_t msgq_release_deallocate(bad_msgq_t *q);
#endif

extern bad_rtos_status_t msgq_acquire(bad_msgq_t *q);
extern bad_rtos_status_t msgq_release(bad_msgq_t *q);
extern bad_rtos_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback, uint32_t delay);
extern bad_rtos_status_t msgq_post_msg(bad_msgq_t *q, uint32_t signal, void *args, uint32_t delay);
extern bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, uint32_t signal, void *args);
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
#define EVENT_BARRIER_FLAGS_VALID_MASK (0x80000000)
#define EVENT_BARRIER_FLAGS_ARE_VALID(flags) (!!((flags) & EVENT_BARRIER_FLAGS_VALID_MASK))
extern bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, uint32_t count);
extern uint32_t event_barrier_wait(bad_event_barrier_t *event_barrier, uint32_t delay);
extern bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier, uint32_t flag);
extern bad_rtos_status_t event_barrier_fire(bad_event_barrier_t *event_barrier, uint32_t flag);
extern bad_rtos_status_t event_barrier_delete(bad_event_barrier_t *event_barrier);
#endif

#ifdef BAD_RTOS_IMPLEMENTATION

#define BAD_RTOS_PRIO_COUNT BAD_RTOS_MAX_TASKS
typedef enum {
    BAD_ISR_OP_MSGQ_WAKE,
    BAD_ISR_OP_SEM_PUT,
    BAD_ISR_OP_TASK_DELAY_CANCEL,
    BAD_ISR_OP_TASK_UNBLOCK,
    BAD_ISR_OP_EVENT_BARRIER_WAKE
}bad_isr_op_t;

typedef struct bad_isr_op_obj{
    struct bad_isr_op_obj * volatile next;
    bad_isr_op_t op_kind;
    void *arg;
    uint32_t pad;
}bad_isr_op_obj_t;

typedef struct {
    bad_isr_op_obj_t * volatile tail;
    bad_isr_op_obj_t * volatile head;
    bad_isr_op_obj_t stub;
}bad_isr_q_t;

typedef struct {
    volatile uint32_t ticks;
    bad_tcb_t * volatile curr;
    bad_tcb_t * volatile next;
    bad_link_node_t delayq;
    uint8_t is_running;
    uint8_t is_unlocked;
    uint32_t ready_bmask;
    bad_link_node_t readyq[BAD_RTOS_PRIO_COUNT];
    bad_link_node_t blockedq;
    bad_isr_q_t isrq;
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

static uint8_t  __attribute__((aligned(_Alignof(bad_isr_op_obj_t)))) gpool_mem[BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES];
static bad_pool_t gpool;

#ifdef BAD_RTOS_USE_KHEAP

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

#define IDLE_TASK_PRIO BAD_RTOS_PRIO_COUNT-1
#define IDLE_TASK_STACK_SIZE 128

TASK_STATIC_STACK(idle, IDLE_TASK_STACK_SIZE)
// Prototypes for asm helpers, for implementation look right above svc_c function, or grep for "ASM stuff"
extern void idle_task();
extern void __first_task_start();

#ifdef BAD_RTOS_USE_SEMAPHORE
extern bad_rtos_status_t __svc_sem_take(bad_sem_t *sem,uint32_t delay);
extern bad_rtos_status_t __svc_sem_put(bad_sem_t *sem);
#endif

static inline uint32_t __attribute__((always_inline)) __get_ipsr();
static inline uint32_t __attribute__((always_inline)) __modify_basepri(uint32_t basepri);
static inline void __attribute__((always_inline)) __restore_basepri(uint32_t basepri);
static inline uint32_t __attribute__((always_inline)) __ldrex(volatile uint32_t* addr);
static inline uint32_t  __attribute__((always_inline)) __strex(uint32_t val,volatile uint32_t* addr);
static inline uint16_t __attribute__((always_inline)) __ldrexh(volatile uint16_t* addr);
static inline uint32_t  __attribute__((always_inline)) __strexh(uint16_t val,volatile uint16_t* addr);
static inline void  __attribute__((always_inline)) __clrex();
static inline void __attribute__((always_inline)) __set_control(uint32_t control);
static inline uint32_t __attribute__((always_inline)) __get_control();
static inline void __attribute__((always_inline)) __dmb();
static inline void __attribute__((always_inline)) __dsb();
static inline void __attribute__((always_inline)) __isb();

#define BAD_OPT_BARRIER __asm__ volatile("":::"memory")
#define BAD_RTOS_STATIC static 

#define BAD_TASK_HANDLE_GEN(gen) ((gen) << 16)
#define BAD_TASK_HANDLE_GET_IDX(handle) ((handle) & 0xFFFF)
#define BAD_TASK_HANDLE_GET_GEN(handle) ((handle) >> 16)
#define BAD_TASK_HANDLE_INVALID_HANDLE(error) ((error) << 16|0xFFFF)

#define BAD_CONTAINER_OF(ptr, type, member) ({ \
_Static_assert(__builtin_types_compatible_p(typeof(*(ptr)), typeof(((type *)0)->member)), \
"Pointer type mismatch in container_of"); \
((type *)( (char *)(ptr) - __builtin_offsetof(type, member) ));\
})


//Linker script symbols
extern uint8_t __kernel_bss;
extern uint8_t __ekernel_bss;

extern uint8_t __kernel_data;
extern uint8_t __ekernel_data;
extern uint8_t __rkernel_data;

extern uint8_t __static_stacks;

//SCB
typedef struct
{
    volatile  uint32_t CPUID;                  
    volatile  uint32_t ICSR;                   
    volatile  uint32_t VTOR;                   
    volatile  uint32_t AIRCR;                  
    volatile  uint32_t SCR;                    
    volatile  uint32_t CCR;                    
    volatile  uint8_t  SHP[12];               
    volatile  uint32_t SHCSR;                  
    volatile  uint32_t CFSR;                   
    volatile  uint32_t HFSR;                   
    volatile  uint32_t DFSR;                   
    volatile  uint32_t MMFAR;                  
    volatile  uint32_t BFAR;                   
    volatile  uint32_t AFSR;                   
    volatile  uint32_t PFR[2];                
    volatile  uint32_t DFR;                    
    volatile  uint32_t ADR;                    
    volatile  uint32_t MMFR[4];               
    volatile  uint32_t ISAR[5];               
    uint32_t RESERVED0[5];
    volatile  uint32_t CPACR;                  
} bad_scb_typedef_t;

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
    BAD_SCB_LOWEST_PRIO = ((1 << BAD_RTOS_PRIO_BITS) - 1),
}bad_scb_interrupt_priority_t;

typedef enum{
    BAD_SCB_FPU_NO_ACCESS = 0,
    BAD_SCB_FPU_PRIV_ACCESS = 5,
    BAD_SCB_FPU_FULL_ACCESS = 15,
}bad_scb_fpu_permission_t;

#define BAD_SCB ((bad_scb_typedef_t *) 0xE000ED00UL)

#define BAD_SCB_ICSR_PENDSVSET                  (0x1U << 28U) 

#define BAD_SCB_CPACR_FPU_SHIFT                 20U
#define BAD_SCB_CPACR_FPU_MASK                  (0xFU << BAD_SCB_CPACR_FPU_SHIFT)

BAD_RTOS_STATIC void __scb_trigger_pendsv(){
    BAD_SCB->ICSR = BAD_SCB_ICSR_PENDSVSET;
}

BAD_RTOS_STATIC void __scb_set_core_interrupt_priority(bad_scb_core_interrupt_t intr, bad_scb_interrupt_priority_t prio){
    BAD_SCB->SHP[intr] = prio << (8U - BAD_RTOS_PRIO_BITS);
}

BAD_RTOS_STATIC void __scb_set_fpu_permission_level(bad_scb_fpu_permission_t perms){
    BAD_SCB->CPACR &= ~(BAD_SCB_CPACR_FPU_MASK);
    BAD_SCB->CPACR |= perms << BAD_SCB_CPACR_FPU_SHIFT;
    __dsb();
    __isb();
} 

#ifdef BAD_RTOS_USE_FPU
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

BAD_RTOS_STATIC void __fpu_init(bad_fpu_features_t features){
    BAD_FPU->FPCCR = features;
    __dsb();
    __isb();
}

#define BAD_RTOS_FPU_SETTINGS (BAD_FPU_FEATURE_ENABLE_LAZY_STACKING|BAD_FPU_FEATURE_ENABLE_AUTO_STACKING)

#endif

#ifdef BAD_RTOS_USE_MPU

typedef struct {
    volatile uint32_t TYPE;                   
    volatile uint32_t CTRL;                   
    volatile uint32_t RNR;                    
    volatile uint32_t RBAR;                   
    volatile uint32_t RASR;                   
    volatile uint32_t RBAR_A1;                
    volatile uint32_t RASR_A1;                
    volatile uint32_t RBAR_A2;                
    volatile uint32_t RASR_A2;                
    volatile uint32_t RBAR_A3;                
    volatile uint32_t RASR_A3;                
} bad_mpu_typedef_t;

#define BAD_MPU_BASE (0xE000ED90UL)
#define BAD_MPU ((bad_mpu_typedef_t *)BAD_MPU_BASE)

#define BAD_MPU_CTRL_ENABLE         (0x1)
#define BAD_MPU_CTRL_DEFAULT_MAP    (0x4)

BAD_RTOS_STATIC void __mpu_enable_with_default_map(){
    __dmb();
    BAD_MPU->CTRL = BAD_MPU_CTRL_ENABLE | BAD_MPU_CTRL_DEFAULT_MAP;
    __dsb();
    __isb();
}

START_TASK_MPU_REGIONS_DEFINITIONS(zeroed) //Fallback zeroed regions array
END_TASK_MPU_REGIONS(zeroed)

#define BAD_RTOS_STACK_RASR BAD_MPU_RASR_ENABLE|(0x4)<<1|BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|BAD_MPU_AP_PRIV_RW_UNPRIV_FAULT

BAD_RTOS_STATIC void __mpu_default_init(){
    //ram region
    BAD_MPU->RNR = 0;
    BAD_MPU->RBAR = 0x20000000;
    BAD_MPU->RASR = BAD_MPU_RASR_ENABLE|(0x10)<<1|BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|BAD_MPU_AP_FULL_ACCESS;
    //preload stack rasr
    BAD_MPU->RNR = 4;
    BAD_MPU->RASR = BAD_RTOS_STACK_RASR;
    //flash region
    BAD_MPU->RNR = 5;
    BAD_MPU->RBAR = 0x08000000;
    BAD_MPU->RASR = BAD_MPU_RASR_ENABLE|(0x12)<<1|BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|BAD_MPU_AP_PRIV_RO_UNPRIV_RO;
    //null adress
    BAD_MPU->RNR = 6;
    BAD_MPU->RBAR = 0x08000000;
    BAD_MPU->RASR = BAD_MPU_RASR_ENABLE|(0x4)<<1|BAD_MPU_TEXSCB_NORMAL_NO_ALLOCATE_WRB_SHAREABLE|BAD_MPU_AP_NO_ACCESS;
    //kernel data structures
    BAD_MPU->RNR = 7;
    BAD_MPU->RBAR = 0x20000000;
    
    BAD_MPU->RASR = BAD_MPU_FIND_SIZE(BAD_MPU_PREV_POW2(&__static_stacks- &__kernel_bss))|
        BAD_MPU_AP_PRIV_RO_UNPRIV_FAULT|BAD_MPU_RASR_XN;
    __mpu_enable_with_default_map();
}
#endif

//Memory helpers
#ifdef BAD_RTOS_USE_KHEAP

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

#ifdef BAD_RTOS_USE_KHEAP

BAD_RTOS_STATIC void* __kernel_alloc(uint32_t size){
    uint32_t closest_order;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    if(closest_order < kernel_buddy.min_order){
        closest_order = kernel_buddy.min_order;
    }
    return __buddy_alloc(&kernel_buddy,closest_order );
}

BAD_RTOS_STATIC void __kernel_free(void *block,uint32_t size){
    uint32_t closest_order;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    if(closest_order < kernel_buddy.min_order){
        closest_order = kernel_buddy.min_order;
    }
    __buddy_free(&kernel_buddy,block,closest_order );
    
}
#endif

BAD_RTOS_STATIC void __tcb_queue_slab_init(){
#if BAD_RTOS_MAX_TASKS < 32
    tcbslab.free_bitmask = (1UL<<(BAD_RTOS_MAX_TASKS))-1;
#else
    tcbslab.free_bitmask = UINT32_MAX;
#endif
}

BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_alloc(){
    if(tcbslab.free_bitmask == 0){
        return 0; 
    }
    uint8_t block_idx = __builtin_ctz(tcbslab.free_bitmask);
    tcbslab.free_bitmask &= ~(1UL<<block_idx);
    return tcbslab.node_arr + block_idx;
}

BAD_RTOS_STATIC uint8_t __tcb_slab_get_idx_from_ptr(bad_tcb_t *block){
    if(tcbslab.node_arr > block || tcbslab.node_arr + BAD_RTOS_MAX_TASKS < block){
        return 0xFF;
    }
    return block - tcbslab.node_arr;
}

BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_get_ptr_from_idx(uint8_t idx){
    if(idx >= BAD_RTOS_MAX_TASKS){
        return 0;
    }
    return tcbslab.node_arr+idx;
}

BAD_RTOS_STATIC void __tcb_slab_free(bad_tcb_t *tcb){
    uint8_t block_idx = __tcb_slab_get_idx_from_ptr(tcb); 
    if(block_idx >= BAD_RTOS_MAX_TASKS){
        return;
    }
    tcbslab.free_bitmask |= (1ULL<<block_idx); 
}

BAD_RTOS_STATIC void *__obj_list_pull_atomic(volatile void* list){
    uint32_t *head;
    do{
        
        head = (uint32_t *)__ldrex(list);
        if(!head){
            __clrex();
            return 0;
        }
    }while(__strex(*head, (volatile uint32_t *)list));
    return head;   
}

BAD_RTOS_STATIC void __obj_list_push_atomic(volatile void *list,void *obj){
    uint32_t *new_head = obj;
    do{
        *new_head = __ldrex(list);
    }while(__strex((uint32_t)new_head, (volatile uint32_t *)list));
}

bad_rtos_status_t pool_init(bad_pool_t *pool,void *mem,uint32_t block_size,uint32_t size_in_bytes){
    if(!pool || !mem || !block_size ||!size_in_bytes || size_in_bytes % block_size){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    pool->mem = mem;
    pool->block_size = block_size;
    BAD_OPT_BARRIER;
    pool->size_in_bytes = size_in_bytes;
    return BAD_RTOS_STATUS_OK;
}

void* pool_alloc(bad_pool_t *pool){
    void *res =__obj_list_pull_atomic(&pool->next);
    if(res){
        return res;
    }
    uint32_t curr;
    do{
        curr = __ldrex(&pool->curr);
        if(curr >= pool->size_in_bytes){
            return 0;
        }
        
    }while(__strex(curr+pool->block_size,&pool->curr));
    return pool->mem + curr;
}

void pool_free(bad_pool_t *pool,void *obj){
    uint8_t *cmp_ptr = obj;
    if(pool->mem > cmp_ptr || pool->mem + pool->size_in_bytes <= cmp_ptr){  
        __builtin_trap();
    }
    __obj_list_push_atomic(pool,obj);
}

void* gpool_alloc(){
    return pool_alloc(&gpool);
}

void gpool_free(void *obj){
    pool_free(&gpool,obj);
}

//Scheduling helpers

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

BAD_RTOS_STATIC uint32_t __get_top_ready_prio(){
    return __builtin_ctz(kernel_cb.ready_bmask);
}

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
        traverse_tcb->counter -= absolute - compound;
    }
    tcb->counter = absolute - compound;
    tcb_delaynode_ptr->prev->next = tcb_delaynode_ptr;
}

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

BAD_RTOS_STATIC void __isr_q_push(bad_isr_q_t *q,bad_isr_op_obj_t* msg){
    bad_isr_op_obj_t *tail ;
#ifdef BAD_RTOS_USE_MPU
    uint32_t kernel_rasr = BAD_MPU->RASR;
    BAD_MPU->RASR = kernel_rasr & ~(BAD_MPU_RASR_ENABLE);
    __dsb();
    __isb();
#endif
    msg->next = 0;
    do{
        tail = (bad_isr_op_obj_t *)__ldrex((volatile uint32_t*)&q->head);
    }while(__strex((uint32_t)msg,(volatile uint32_t *)&q->head));
    tail->next = msg;
    __dmb();
#ifdef BAD_RTOS_USE_MPU
    BAD_MPU->RASR = kernel_rasr;
    __dsb();
#endif
}

BAD_RTOS_STATIC bad_isr_op_obj_t *__isr_q_pop(bad_isr_q_t *q){
    bad_isr_op_obj_t *next =q->tail->next;
    bad_isr_op_obj_t *tail = q->tail;
    if(q->tail == &q->stub){
        if(!next){
            return 0;
        }
        q->tail = next;
        tail = next;
        next = next->next;
    }
    
    if(next){
        q->tail = next;
        return tail;
    }
    
    if(q->head != tail){
        return 0; //not possible in the current usecase but nice to have
    }
    
    __isr_q_push(q,&q->stub);
    next = tail->next;
    if(!next){
        return 0;//not possible in the current usecase but nice to have
    }
    q->tail = next;
    return tail;
    
}

BAD_RTOS_STATIC bad_rtos_status_t __kernel_notify(bad_isr_op_t op,void *arg){
    bad_isr_op_obj_t *message = gpool_alloc();
    if(!message){
        return BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    message->op_kind = op;
    message->arg = arg;
    __dmb();
    __isr_q_push(&kernel_cb.isrq,message);
    __scb_trigger_pendsv();
    return BAD_RTOS_STATUS_OK; 
}

BAD_RTOS_STATIC void __sched_update(bad_tcb_t *tcb){
    kernel_cb.next = tcb;
    tcb->misc = BAD_RTOS_MISC_RUNNING;
}

BAD_RTOS_STATIC void __sched_try_update(){
    uint32_t top_ready_prio = __get_top_ready_prio();
    if(top_ready_prio < kernel_cb.curr->raised_priority && kernel_cb.is_unlocked){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}

BAD_RTOS_STATIC void __sched_try_preempt(bad_tcb_t *tcb){
    
    if(tcb->raised_priority < kernel_cb.curr->raised_priority && kernel_cb.is_unlocked){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(tcb);
    }else {
        __readyq_enqueue(tcb);
    }
    
}

static void __attribute__((used)) __handle_systick_event(bad_systick_status_t status){
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
    
    if(kernel_cb.ready_bmask && kernel_cb.is_unlocked && 
       top_ready_prio + (status == BAD_SYSTICK_DELAY_WAKE_PENDING)
       <= kernel_cb.curr->raised_priority ){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}

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

//Core isr api implementations
bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t handle){
    if(!__get_ipsr()){
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    if(!handle || !tcb || tcb->generation != BAD_TASK_HANDLE_GET_GEN(handle)){
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    return __kernel_notify(BAD_ISR_OP_TASK_UNBLOCK,(void*)handle);
}

bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t handle){
    if(!__get_ipsr()){
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    if(!handle || !tcb || tcb->generation != BAD_TASK_HANDLE_GET_GEN(handle)){
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    return __kernel_notify(BAD_ISR_OP_TASK_DELAY_CANCEL,(void*)handle);
}

//Core api implementations
BAD_RTOS_STATIC bad_task_handle_t __task_make(bad_task_descr_t *args){
    bad_rtos_status_t status = BAD_RTOS_STATUS_BAD_PARAMETERS;
    bad_tcb_t *new_task = 0;
    
    if(args->stack_size < 64 || args->stack_size % 32 || args->base_priority >= IDLE_TASK_PRIO){
        goto exit_error;
    }
    
    new_task = __tcb_slab_alloc();
    if(!new_task){
        status = BAD_RTOS_STATUS_ALLOC_FAIL;
        goto exit_error;
    }
    
#ifdef BAD_RTOS_USE_MSGQ
    if(args->assigned_msgq){
        if(args->assigned_msgq->owner){
            status = BAD_RTOS_STATUS_ALREADY_BOUND;
            goto err_free_tcb;
        }
        if(!args->assigned_msgq->capacity){
            goto err_free_tcb;
        }
        new_task->msgq_owner = 1;
        args->assigned_msgq->owner = new_task;
    }else{
        new_task->msgq_owner = 0;
    }
#endif
    
    new_task->stack_size = args->stack_size;
    new_task->dyn_stack = 0;
    
    if(args->stack){
        new_task->stack = args->stack;
        new_task->dyn_stack = 0;
    }else{
#if defined(BAD_RTOS_USE_KHEAP)
        new_task->stack = __kernel_alloc(args->stack_size);
        if(!new_task->stack){
            status = BAD_RTOS_STATUS_ALLOC_FAIL;
            goto err_release_msgq;
        }
        new_task->dyn_stack = 1;
#else
        goto err_release_msgq; // No heap and no stack provided
#endif
    }
    
#ifdef BAD_RTOS_USE_MPU
    if(args->regions){
        new_task->regions = args->regions;
    }else{
        new_task->regions = zeroed_regions;
    }
#endif
    
    new_task->entry = args->entry;
    new_task->base_priority = args->base_priority;
    new_task->raised_priority = args->base_priority;
    new_task->ticks_to_change = args->ticks_to_change;
    new_task->counter = args->ticks_to_change;
    
    uint32_t *stack_top = (uint32_t *)(new_task->stack + args->stack_size);
    new_task->sp = __init_stack(new_task->entry, stack_top, args->args);
    
    if(kernel_cb.is_running){
        __sched_try_preempt(new_task);
    }else{
        __readyq_enqueue(new_task);
    }
    
    uint8_t idx = __tcb_slab_get_idx_from_ptr(new_task);
    return (bad_task_handle_t)(idx | (new_task->generation << 16));
    
    err_release_msgq:
#ifdef BAD_RTOS_USE_MSGQ
    if(args->assigned_msgq){
        args->assigned_msgq->owner = 0;
    }
#endif
    err_free_tcb:
    __tcb_slab_free(new_task);
    exit_error:
    return BAD_TASK_HANDLE_INVALID_HANDLE(status);
}

BAD_RTOS_STATIC void  __task_block(){
    __enqueue_head(&kernel_cb.blockedq, kernel_cb.curr,BAD_RTOS_MISC_BLOCKEDQ_MEMBER);
    __sched_update(__readyq_dequeue_head());
}

BAD_RTOS_STATIC bad_rtos_status_t __task_unblock(bad_task_handle_t handle){
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    if(!handle||!tcb || tcb->generation != BAD_TASK_HANDLE_GET_GEN(handle)){
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
    if(!handle ||!tcb || tcb->generation != BAD_TASK_HANDLE_GET_GEN(handle)){
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
    if(top_ready_prio == kernel_cb.curr->raised_priority && kernel_cb.is_unlocked){
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
        return BAD_RTOS_STATUS_OK;
    }
    else{
        return BAD_RTOS_STATUS_CANT_YIELD;
    }
}

BAD_RTOS_STATIC void __task_finish(){
#ifdef BAD_RTOS_USE_MUTEX
    if(kernel_cb.curr->mutex_count){ //trap when task want to finish without releasing mutexes
        __builtin_trap();
        return;
    }
#endif 
#ifdef BAD_RTOS_USE_MSGQ
    if(kernel_cb.curr->msgq_owner){
        __builtin_trap();
        return;
    }
#endif
#if defined (BAD_RTOS_USE_KHEAP)
    if(kernel_cb.curr->dyn_stack){ //free the dynamically allocated stack 
        __kernel_free((void*)kernel_cb.curr->stack,kernel_cb.curr->stack_size);
    }
#endif
    __sched_update(__readyq_dequeue_head());
    kernel_cb.curr->generation++;
    __tcb_slab_free(kernel_cb.curr);//free the the tcb used by task
}

BAD_RTOS_STATIC void __task_delay(uint32_t delay,cbptr cb, void* args){
    kernel_cb.curr->cbptr =cb;
    kernel_cb.curr->args = args;
    __delayq_enqueue(kernel_cb.curr,delay);
    __sched_update(__readyq_dequeue_head());
    
}

BAD_RTOS_STATIC void __kernel_start(){
    if(kernel_cb.is_running){
        __builtin_trap();
    }
    
    pool_init(&gpool,gpool_mem,sizeof(bad_isr_op_obj_t),BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES);
    kernel_cb.is_running = 1;
    kernel_cb.is_unlocked = 1;
    __set_control(0x1);
    __restore_basepri(0);
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_SCB_LOWEST_PRIO);
    kernel_cb.curr = __readyq_dequeue_head();
    __asm volatile("b __init_second_stage");
}

BAD_RTOS_STATIC uint32_t __sched_lock(){
    uint32_t lock = kernel_cb.is_unlocked;
    kernel_cb.is_unlocked = 0;
    return lock;
}

BAD_RTOS_STATIC void __sched_unlock(uint32_t key){
    kernel_cb.is_unlocked = key;
    __sched_try_update();
}

//Startup code
BAD_RTOS_STATIC void __kernel_sections_init(){
    uint32_t *src = (uint32_t *)&__kernel_bss;
    uint32_t *end = (uint32_t *)&__ekernel_bss;
    while (src < end) {
        *src++ = 0; 
    }
    
    src = (uint32_t *)&__rkernel_data;
    uint32_t *dest = (uint32_t *)&__kernel_data;
    end = (uint32_t *)&__ekernel_data;
    while (dest<end) {
        *dest++ = *src++;
    }
}

BAD_RTOS_STATIC void __interrupt_init(){
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_SCB_PRIO0);
    __scb_set_core_interrupt_priority(BAD_SCB_PENDSV_INTR, BAD_SCB_LOWEST_PRIO);
}

BAD_RTOS_STATIC void __readyq_init(){
    for (uint32_t i = 0; i < BAD_RTOS_PRIO_COUNT; i++){
        kernel_cb.readyq[i].next = &kernel_cb.readyq[i];
        kernel_cb.readyq[i].prev = &kernel_cb.readyq[i];
    }
}

BAD_RTOS_STATIC void __irq_q_init(){
    kernel_cb.isrq.head = &kernel_cb.isrq.stub;
    kernel_cb.isrq.tail = &kernel_cb.isrq.stub;
}

BAD_RTOS_STATIC void __idle_task_init(){
    bad_tcb_t *idle_tcb = __tcb_slab_alloc(); //always idx 0
    idle_tcb->stack = idle_stack;
    idle_tcb->stack_size = IDLE_TASK_STACK_SIZE;
    idle_tcb->base_priority = IDLE_TASK_PRIO;
    idle_tcb->raised_priority = IDLE_TASK_PRIO;
    idle_tcb->ticks_to_change = UINT32_MAX;
    idle_tcb->entry = idle_task;
    idle_tcb->counter = UINT32_MAX;
#ifdef BAD_RTOS_USE_MPU
    idle_tcb->regions = zeroed_regions;
#endif
    idle_tcb->sp = __init_stack(idle_task, (uint32_t *)(idle_stack + IDLE_TASK_STACK_SIZE),0);
    __readyq_enqueue(idle_tcb);
}

// declaration for user init function
extern void bad_user_init();

void bad_rtos_start(){
    __restore_basepri(1 << (8 - BAD_RTOS_PRIO_BITS));
    __kernel_sections_init();
    __irq_q_init();
    __readyq_init();
    __interrupt_init();
    __tcb_queue_slab_init();
    __idle_task_init();
#ifdef BAD_RTOS_USE_KHEAP
    __buddy_init(&kernel_buddy, kheap, kfreelist, KMIN_ORDER, KMAX_ORDER, kbitmask);
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
    bad_user_init();
    __first_task_start();
}

//Synchro helpers

BAD_RTOS_STATIC bad_tcb_t* __synchro_wake(bad_link_node_t *q,cbptr cb,bad_rtos_status_t status){
    bad_tcb_t *tcb = __prio_list_dequeue_head(q);
    if(!tcb){
        return 0;
    }
    if(tcb->cbptr == cb){
        __delayq_dequeue(tcb);
        tcb->cbptr = 0;
        tcb->args = 0;
    }
    *(tcb->sp+9) = status;
    __sched_try_preempt(tcb);
    return tcb;
}

BAD_RTOS_STATIC void __synchro_wake_all(bad_link_node_t *q,cbptr cb, uint32_t status){
    bad_link_node_t *traverse = q->next;
    bad_tcb_t *traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    while(traverse){
        *(traverse_tcb->sp+9) = status; 
        if(traverse_tcb->cbptr == cb){
            traverse_tcb->cbptr = 0;
            traverse_tcb->args = 0;
            __delayq_dequeue(traverse_tcb);
        }
        traverse = traverse->next;
        __readyq_enqueue(traverse_tcb);
        traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    }
    *q = (bad_link_node_t){0};
    __sched_try_update();
}

BAD_RTOS_STATIC  bad_rtos_status_t __synchro_block(bad_link_node_t *q, cbptr cb, uint32_t delay, bad_rtos_misc_t misc){
    if(delay == UINT32_MAX){
        return BAD_RTOS_STATUS_WOULD_BLOCK;
    }
    
    if(delay){
        kernel_cb.curr->args = q; //every synchro obj has blockedq as first element
        kernel_cb.curr->cbptr = cb;
        __delayq_enqueue( kernel_cb.curr, delay);
    }
    
    __prio_list_enqueue(q,kernel_cb.curr, misc);
    __sched_update(__readyq_dequeue_head());
    return BAD_RTOS_STATUS_OK;
}

//Synchro objects api implementations
#ifdef BAD_RTOS_USE_MSGQ

BAD_RTOS_STATIC void __msgq_timeout_cb(bad_task_handle_t handle ,void *msgq){
    (void)msgq;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    __remove_entry(tcb,BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

#ifdef BAD_RTOS_USE_KHEAP

BAD_RTOS_STATIC bad_rtos_status_t __msgq_acquire_allocate(bad_msgq_t *q,uint16_t capacity){
    if(!q || (capacity & (capacity - 1))){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    if(kernel_cb.curr->msgq_owner){
        return BAD_RTOS_STATUS_ALREADY_BOUND;
    }
    kernel_cb.curr->msgq_owner = 1;
    q->msgs = __kernel_alloc(capacity);
    q->owner = kernel_cb.curr;
    q->dynamic = 1;
    q->blockedq = (bad_link_node_t){0};
    q->head = q->tail = 0;
    BAD_OPT_BARRIER;
    q->capacity = capacity;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __msgq_release_deallocate(bad_msgq_t *q){
    if(!q || !q->dynamic){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner != kernel_cb.curr){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    kernel_cb.curr->msgq_owner = 0;
    uint32_t capacity = q->capacity;
    q->capacity = 0;
    BAD_OPT_BARRIER;
    __kernel_free(q->msgs,capacity);
    __synchro_wake_all(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_DELETED);
    *q = (bad_msgq_t){0};
    return BAD_RTOS_STATUS_OK;
}

#endif

BAD_RTOS_STATIC bad_rtos_status_t __msgq_acquire(bad_msgq_t *q){
    if(!q){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    if(kernel_cb.curr->msgq_owner){
        return BAD_RTOS_STATUS_ALREADY_BOUND;
    }
    
    q->owner = kernel_cb.curr;
    kernel_cb.curr->msgq_owner = 1;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __msgq_release(bad_msgq_t *q){
    if(!q){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner != kernel_cb.curr){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    q->owner = 0;
    kernel_cb.curr->msgq_owner = 0;
    volatile uint32_t *atomic_update = (volatile uint32_t *)&q->head; 
    *atomic_update = 0;
    __synchro_wake_all(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_DELETED);
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t __msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback,uint32_t delay){
    if(!q || !writeback){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!q->capacity){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    if(q->owner != kernel_cb.curr){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    if(q->tail==q->head){
        return __synchro_block(&q->blockedq,__msgq_timeout_cb,delay, BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER);
    }
    
    *writeback = *(q->msgs+q->tail);
    BAD_OPT_BARRIER;
    bad_tcb_t *tcb = __synchro_wake(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_OK);
    if(tcb){
        uint16_t next_tail = (q->tail+1) & (q->capacity-1);
        uint16_t next_head = (q->head+1) & (q->capacity-1);
        uint32_t signal = *(tcb->sp+10);
        void *args = (void *)*(tcb->sp+11);
        bad_msg_block_t *block = q->msgs + q->head;
        block->signal = signal;
        block->args = args;
        BAD_OPT_BARRIER;
        volatile uint32_t *atomic_update = (volatile uint32_t *)&q->head;
        *atomic_update = next_head | (next_tail << 16);
    }else{
        q->tail = (q->tail+1) & (q->capacity-1);
    }
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __msgq_try_wake(bad_msgq_t *q){
    bad_tcb_t *tcb = __synchro_wake(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_OK);
    if(tcb){ 
        bad_msg_block_t *writeback = (bad_msg_block_t *)*(tcb->sp + 10);
        *writeback = *(q->msgs+q->tail);
        BAD_OPT_BARRIER;
        q->tail = (q->tail+1) & (q->capacity-1);
    }
}

bad_rtos_status_t __msgq_post_msg(bad_msgq_t *q, uint32_t signal, void *args,uint32_t delay){
    uint16_t head,next_head;
    
    if(!q){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!q->capacity){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    do{
        
        head = __ldrexh(&q->head);
        next_head = (head+1) & (q->capacity-1);
        if(q->tail == next_head){
            __clrex();
            return __synchro_block(&q->blockedq,__msgq_timeout_cb,delay, BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER);
        }
        
    }while(__strexh(next_head, &q->head)); 
    
    bad_msg_block_t* block = q->msgs+head;
    
    block->signal = signal;
    block->args = args;
    if(head == q->tail){
        __msgq_try_wake(q); 
    }
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, uint32_t signal, void *args){
    if(!__get_ipsr()){ 
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    if(!q){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!q->capacity){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    uint32_t head,next_head;
    do{
        
        head = __ldrexh(&q->head);
        next_head = (head+1) & (q->capacity-1);
        if(q->tail == next_head){
            __clrex();
            return BAD_RTOS_STATUS_WOULD_BLOCK;
        }
        
    }while(__strexh(next_head, &q->head)); 
    
    bad_msg_block_t* block = q->msgs+head;
    
    block->signal = signal;
    block->args = args;
    if(q->tail == head){// we may have preempted the consumer mid block, need to check in pendsv
        return __kernel_notify(BAD_ISR_OP_MSGQ_WAKE,q);
    }
    return BAD_RTOS_STATUS_OK;
    
}

#endif

#ifdef BAD_RTOS_USE_MUTEX
bad_rtos_status_t mutex_init(bad_mutex_t *mut){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    *mut = (bad_mutex_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __mutex_timeout_cb(bad_task_handle_t handle ,void *mutex){
    (void)mutex;
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
    
    
    __synchro_wake_all(&mut->blockedq,__mutex_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    *mut = (bad_mutex_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

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
        mut->rec_takes++;
        return BAD_RTOS_STATUS_OK;
    }
    
    if(kernel_cb.curr->raised_priority < mut->owner->raised_priority){
        mut->owner->raised_priority = kernel_cb.curr->raised_priority;
        __mutex_update_owner_pos(mut->owner);
    }
    
    return __synchro_block(&mut->blockedq,__mutex_timeout_cb,delay, BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER);
}   

BAD_RTOS_STATIC bad_rtos_status_t __mutex_put(bad_mutex_t *mut){
    if(!mut){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(kernel_cb.curr!= mut->owner){
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    if(mut->rec_takes){
        mut->rec_takes--;
        return BAD_RTOS_STATUS_OK;
    }
    
    mut->owner =  __synchro_wake(&mut->blockedq,__mutex_timeout_cb,BAD_RTOS_STATUS_OK);
    
    if(!--kernel_cb.curr->mutex_count){
        kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
    }
    
    if(!mut->owner){ 
        return BAD_RTOS_STATUS_OK; 
    }
    mut->owner->mutex_count++;    
    
    return BAD_RTOS_STATUS_OK;
}
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
bad_rtos_status_t sem_init(bad_sem_t *sem, uint32_t reset_value){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    sem->blockedq = (bad_link_node_t){0};
    sem->counter = reset_value;
    sem->init_flag = 1;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __sem_timeout_cb(bad_task_handle_t handle ,void *semaphore){
    (void)semaphore;
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
    
    if(!sem->blockedq.next){
        return BAD_RTOS_STATUS_OK;
    }
    
    __synchro_wake_all(&sem->blockedq,__sem_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    *sem = (bad_sem_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t sem_take(bad_sem_t *sem,uint32_t delay){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    uint32_t counter;
    do{
        counter = __ldrex(&sem->counter);
        if(!counter){
            __clrex();
            if(delay == UINT32_MAX){
                return BAD_RTOS_STATUS_WOULD_BLOCK;
            }
            return __svc_sem_take(sem,delay);
        }
    }while(__strex(counter-1, &sem->counter));
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t sem_put(bad_sem_t *sem){
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    uint32_t counter;
    do{
        counter = __ldrex(&sem->counter);
        if(((volatile typeof(sem->blockedq) *)&sem->blockedq)->next){
            __clrex();
            return __svc_sem_put(sem);
        }
    }while(__strex(counter+1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_put(bad_sem_t *sem){
    bad_tcb_t *tcb = __synchro_wake(&sem->blockedq,__sem_timeout_cb,BAD_RTOS_STATUS_OK);
    if(tcb){
        return BAD_RTOS_STATUS_OK;
    }
    uint32_t counter;
    do{
        counter = __ldrex(&sem->counter);
    }while(__strex(counter+1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_take(bad_sem_t *sem, uint32_t delay){
    if(!sem->counter){
        return __synchro_block(&sem->blockedq, __sem_timeout_cb, delay, BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER);
    }
    uint32_t counter;
    do{
        counter = __ldrex(&sem->counter);
    }while(__strex(counter-1, &sem->counter));
    return BAD_RTOS_STATUS_OK;
} 

bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem){
    if(!__get_ipsr()){
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    if(!sem){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    uint32_t counter;
    do{
        counter = __ldrex(&sem->counter);
        if(!counter){
            __clrex();
            return __kernel_notify(BAD_ISR_OP_SEM_PUT,sem);
        }
        
    }while(__strex(counter+1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}

#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER

static void __event_barrier_timeout_cb(bad_task_handle_t handle ,void *event_barrier){
    (void)event_barrier;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(BAD_TASK_HANDLE_GET_IDX(handle));
    __remove_entry(tcb,BAD_RTOS_MISC_EVENT_BARRIER_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, uint32_t count){
    if(!event_barrier|| !count || count >= 32){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(event_barrier->count && event_barrier->count != 32){
        return BAD_RTOS_STATUS_IN_USE;
    }
    *event_barrier = (bad_event_barrier_t){0};
    BAD_OPT_BARRIER;
    event_barrier->count = count;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC uint32_t __event_barrier_wait(bad_event_barrier_t *event_barrier,uint32_t delay){
    if(!event_barrier){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!event_barrier->count){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    if(event_barrier->count == 32){
        return BAD_RTOS_STATUS_FIRED;
    }
    
    return __synchro_block(&event_barrier->blockedq,__event_barrier_timeout_cb,delay, BAD_RTOS_MISC_EVENT_BARRIER_BLOCKEDQ_MEMBER);
}

bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier,uint32_t flag){
    if(!__get_ipsr()){
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    if(!event_barrier ||!flag ||flag == EVENT_BARRIER_FLAGS_VALID_MASK){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!event_barrier->count){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    uint32_t flags,new_flags;
    do{
        
        flags = __ldrex(&event_barrier->flags);
        if(event_barrier->count == 32){
            __clrex();
            return BAD_RTOS_STATUS_FIRED;
        }       
        new_flags = flags | flag;
        if(new_flags == flags ){
            __clrex();
            return BAD_RTOS_STATUS_OK;
        }
        
    }while(__strex(new_flags, &event_barrier->flags));
    
    if(__builtin_popcount(new_flags) == event_barrier->count){
        event_barrier->count = 32;
        BAD_OPT_BARRIER;
        event_barrier->flags = new_flags;//report correct flags on wakeup
        return __kernel_notify(BAD_ISR_OP_EVENT_BARRIER_WAKE,event_barrier);
    }
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __event_barrier_wake(bad_event_barrier_t *event_barrier){
    uint32_t flags = event_barrier->flags;
    __synchro_wake_all(&event_barrier->blockedq,__event_barrier_timeout_cb,flags|EVENT_BARRIER_FLAGS_VALID_MASK);
}

BAD_RTOS_STATIC bad_rtos_status_t __event_barrier_fire(bad_event_barrier_t *event_barrier,uint32_t flag){
    if(!event_barrier ||!flag ||flag == EVENT_BARRIER_FLAGS_VALID_MASK){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!event_barrier->count){
        return BAD_RTOS_STATUS_NOT_INITIALISED; 
    }    
    
    uint32_t flags,new_flags;
    do{
        
        flags = __ldrex(&event_barrier->flags);
        if(event_barrier->count == 32){
            __clrex();
            return BAD_RTOS_STATUS_FIRED;
        }       
        new_flags = flags | flag;
        if(new_flags == flags){
            __clrex();
            return BAD_RTOS_STATUS_OK;
        }    
    }while(__strex(new_flags, &event_barrier->flags));
    
    if(__builtin_popcount(new_flags) == event_barrier->count){
        event_barrier->count = 32; 
        BAD_OPT_BARRIER;
        event_barrier->flags = new_flags;//report correct flags on wakeup
        __event_barrier_wake(event_barrier);
    }
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __event_barrier_delete(bad_event_barrier_t *event_barrier){
    if(!event_barrier){
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    if(!event_barrier->count){
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    __synchro_wake_all(&event_barrier->blockedq,__event_barrier_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    *event_barrier = (bad_event_barrier_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

#endif
//ISRS

static void __attribute__((used)) __svc_c(uint8_t svc, uint32_t* stack){
    switch (svc) {
        //start first task
        
        case 0x2:{
            stack[0]=__task_unblock(stack[0]);
            break;
        }
        case 0x3:{
            stack[0] =__task_delay_cancel(stack[0]);
            break;
        }
        case 0x4:{
            __task_finish();
            stack[0] = BAD_RTOS_STATUS_CANT_FINISH;
            break;
        }
        case 0x5:{
            stack[0] = __task_yield();
            break;
        }
        case 0x6:{
            __task_block();            
            stack[0] = BAD_RTOS_STATUS_OK;              
            break;
        }
        case 0x7:{
            __task_delay(stack[0], (cbptr) stack[1] ,(void*)stack[2]);
            stack[0] = BAD_RTOS_STATUS_OK;
            break;
        }
        
        
#ifdef BAD_RTOS_USE_SEMAPHORE
        case 0xA:{
            stack[0] = __sem_put((bad_sem_t *)stack[0]);
            break;
        }
        case 0xB:{
            stack[0] = __sem_take((bad_sem_t*)stack[0] , stack[1]);
            break;
        }
        case 0xC:{
            stack[0] = __sem_delete((bad_sem_t*)stack[0]);
            break;
        }
#endif
        
#ifdef BAD_RTOS_USE_MUTEX
        case 0xD:{
            stack[0] = __mutex_put((bad_mutex_t*)stack[0]);
            break;
        }
        case 0xE:{
            stack[0] = __mutex_take((bad_mutex_t*)stack[0], stack[1]);
            break;
        }
        case 0xF:{
            stack[0] = __mutex_delete((bad_mutex_t*)stack[0]);
            break;
        }       
#endif
        
#ifdef BAD_RTOS_USE_MSGQ
        case 0x10:{
            stack[0] = __msgq_post_msg((bad_msgq_t *)stack[0],stack[1],(void*)stack[2],stack[3]);
            break;
        }
        case 0x11:{
            stack[0] = __msgq_pull_msg((bad_msgq_t *)stack[0],(bad_msg_block_t *)stack[1],stack[2]);
            break;
        }
        case 0x12:{
            stack[0] = __msgq_acquire((bad_msgq_t *)stack[0]);
            break;
        }
        case 0x13:{
            stack[0] = __msgq_release((bad_msgq_t *)stack[0]);
            break;
        }
#ifdef BAD_RTOS_USE_KHEAP
        case 0x14:{
            stack[0] = __msgq_acquire_allocate((bad_msgq_t *)stack[0],stack[1]);
            break;
        }
        case 0x15:{
            stack[0] = __msgq_release_deallocate((bad_msgq_t *)stack[0]);
            break;
        }
#endif
#endif
#ifdef BAD_RTOS_USE_EVENT_BARRIER
        case 0x16:{
            stack[0] = __event_barrier_wait((bad_event_barrier_t *)stack[0],stack[1]);
            break;
        }        
        case 0x17:{
            stack[0] = __event_barrier_fire((bad_event_barrier_t *)stack[0],stack[1]);
            break;
        }        
        case 0x18:{
            stack[0] = __event_barrier_delete((bad_event_barrier_t *)stack[0]);
            break;
        }
#endif
        case 0xF0:{
            stack[0] = __sched_lock();
            break;
        }
        case 0xF1:{
            __sched_unlock(stack[0]);
            break;
        }
        
#ifdef BAD_RTOS_USE_KHEAP
        case 0xF2:{
            stack[0]=(uint32_t)__kernel_alloc(stack[0]);
            break;
        }
        case 0xF3:{
            __kernel_free((void*)stack[0], stack[1]);
            break;
        } 
#endif
        case 0xF4:{
            stack[0] = (uint32_t)__task_make((bad_task_descr_t*)stack[0]);
            break;
        }
        case 0xF5:{
            __kernel_start();
            break;
        }
        default:{
            __builtin_unreachable();
        }
    }
}

static void __attribute__((used)) __pendsv_c(){
    bad_isr_op_obj_t *msg;
    while((msg = __isr_q_pop(&kernel_cb.isrq))){
        switch(msg->op_kind){
            case BAD_ISR_OP_TASK_DELAY_CANCEL:{
                __task_delay_cancel((bad_task_handle_t)(msg->arg));
                break;
            }
            
            case BAD_ISR_OP_TASK_UNBLOCK:{
                __task_unblock((bad_task_handle_t)(msg->arg));
                break;
            }
            
#ifdef BAD_RTOS_USE_SEMAPHORE
            case BAD_ISR_OP_SEM_PUT:{
                __sem_put((bad_sem_t *)(msg->arg));
                break;
            }
#endif
#ifdef BAD_RTOS_USE_MSGQ
            case BAD_ISR_OP_MSGQ_WAKE:{
                __msgq_try_wake((bad_msgq_t *)(msg->arg));
                break;
            }
#endif
            
#ifdef BAD_RTOS_USE_EVENT_BARRIER
            case BAD_ISR_OP_EVENT_BARRIER_WAKE:{
                __event_barrier_wake((bad_event_barrier_t *)(msg->arg));
                break;
            }
#endif
            default:{
                __builtin_unreachable();
            }
        }
        gpool_free(msg);
    }
}

// ASM stuff

void __attribute__((naked)) BAD_RTOS_SVC_HANDLER_NAME(){ 
    __asm__ volatile(
                     "svc_isr:               \n"
                     "tst lr, #4             \n"
                     "ite eq                 \n"
                     "mrseq r1, msp          \n"
                     "mrsne r1, psp          \n"
                     "ldr r3, [r1,#24]       \n"
                     "ldrb r0, [r3,#-2]      \n"
                     "ldr r3,=%0             \n"
                     "ldrb r3,[r3]           \n"
                     "cbz r3,.L_sched_locked \n"
                     ".L_svc_cont:           \n"
                     "push {r7,lr}           \n"
                     ".cfi_adjust_cfa_offset 8\n"
                     ".cfi_rel_offset r7, 0  \n"
                     ".cfi_rel_offset lr, 4 \n"
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%2            \n"
                     "ldr r3,[r12,#8]        \n"
                     "bic r2,r3,#1           \n"
                     "str r2,[r12,#8]        \n"
                     "push {r3,r12}          \n"
                     ".cfi_adjust_cfa_offset 8\n"
                     ".cfi_rel_offset r3, 0  \n"
                     ".cfi_rel_offset r12, 4 \n"
                     
#endif
                     "bl __svc_c             \n"
#ifdef BAD_RTOS_USE_MPU
                     "pop {r3,r12}            \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r3         \n"
                     ".cfi_restore r12        \n"
#endif
                     "pop {r7,lr}            \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r7        \n"
                     ".cfi_restore lr        \n"
                     "b __try_context_switch \n"
                     ".L_sched_locked:       \n"//todo : produce correct debug info, this works just because its 0 sum
                     "cmp r0,#0xF0           \n"
                     "bhs .L_svc_cont        \n"
                     "mov r0,%1              \n"
                     "str r0,[r1]            \n"
                     "bx lr                  \n"
                     ".ltorg                 \n"
                     :
                     :"i"(&kernel_cb.is_unlocked),"i"(BAD_RTOS_STATUS_SCHED_LOCKED)
#ifdef BAD_RTOS_USE_MPU
                     ,"i"(&BAD_MPU->RNR)
#endif
                     :
                     );
}

//pendsv isr
void __attribute__((naked)) BAD_RTOS_PENDSV_HANDLER_NAME(){ 
    __asm__ volatile(
                     "push {r7,lr}             \n"
                     ".cfi_adjust_cfa_offset 8 \n"
                     ".cfi_rel_offset r7, 0    \n"
                     ".cfi_rel_offset lr, 4    \n"
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%0              \n"
                     "ldr r3,[r12,#8]          \n"
                     "bic r0,r3,#1             \n"
                     "str r0,[r12,#8]          \n"
                     "push {r3,r12}            \n"
                     ".cfi_adjust_cfa_offset 8 \n"
                     ".cfi_rel_offset r3, 0    \n"
                     ".cfi_rel_offset r12, 4   \n"
#endif
                     "bl __pendsv_c            \n"
#ifdef BAD_RTOS_USE_MPU
                     "pop {r3,r12}             \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r3          \n"
                     ".cfi_restore r12         \n"
#endif
                     "pop {r7,lr}              \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r7          \n"
                     ".cfi_restore lr          \n"
                     "b __try_context_switch   \n"
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
                     
                     "ldr r2,=%0               \n"
                     "ldrb r0,[r2,#20]         \n"
                     "cbz r0, exit             \n"
                     "b cont                   \n"
                     "exit:                    \n"
                     "bx lr                    \n"
                     "cont:                    \n"
                     "push {r7,lr}             \n"
                     ".cfi_adjust_cfa_offset 8 \n"
                     ".cfi_rel_offset r7, 0    \n"
                     ".cfi_rel_offset lr, 4    \n"
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%1              \n"
                     "ldr r3,[r12,#8]          \n"
                     "bic r0,r3,#1             \n"
                     "str r0,[r12,#8]          \n"
                     "push {r3,r12}            \n"
                     ".cfi_adjust_cfa_offset 8 \n" // Stack pointer moved 8 bytes
                     ".cfi_rel_offset r3, 0    \n"   // r7 is at SP + 0
                     ".cfi_rel_offset r12, 4   \n"
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
                     "ldr r2,[r2,#16]          \n"
                     "cbnz r2,.L_nz_delayq     \n"
                     "b .L_skip_delayq         \n"
                     ".L_nz_delayq:            \n"
                     "ldr r1,[r2,#12]          \n"
                     "subs r1,#1               \n"
                     "str r1,[r2,#12]          \n"
                     "it eq                    \n"
                     "orreq r0,#2              \n"
                     ".L_skip_delayq:          \n"
                     "cbnz r0,.L_handle_event  \n"
                     ".cfi_remember_state      \n"
#ifdef BAD_RTOS_USE_MPU
                     "pop {r3,r12}             \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r3          \n"
                     ".cfi_restore r12         \n"
                     "str r3,[r12,#8]          \n"
                     "dsb                      \n"
#endif
                     "pop {r7,pc}              \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r7          \n"
                     ".cfi_restore pc          \n"
                     
                     ".L_handle_event:         \n"
                     ".cfi_restore_state       \n"
                     "bl __handle_systick_event\n"
                     
                     "pop {r3,r12}             \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r3          \n"
                     ".cfi_restore r12         \n"
                     
                     "pop {r7,lr}          \n"
                     ".cfi_adjust_cfa_offset -8\n"
                     ".cfi_restore r7          \n"
                     ".cfi_restore lr          \n"
                     
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
static void __attribute__((naked,used)) __try_context_switch(){
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
#ifdef BAD_RTOS_USE_MPU
                     "ldr r1,[r2,#4]           \n"
                     "orr r1,#0x14             \n"
                     "str r1,[r12,#4]          \n"
                     "mov r1,#1                \n"
                     "str r1,[r12]             \n"
                     "ldr r2,[r2,#48]          \n"
                     "add r1,r12,#4            \n"
                     "ldmia r2!,{r5-r10}       \n"
                     "stmia r1!,{r5-r10}       \n"
                     "mov r2,#7                \n"
                     "str r2,[r12]             \n"
                     "str r3,[r12,#8]          \n"
                     "dsb                      \n"
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
void __attribute__((naked)) __init_second_stage(){
    __asm__ volatile(
                     "ldr r1,=__estack           \n"
                     "msr msp,r1                 \n"
                     "ldr r1,=kernel_cb          \n"
                     "ldr r1,[r1,#4]             \n"
                     "ldr r0,[r1]                \n"
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%0                \n"
                     "ldr r2,[r1,#4]             \n"
                     "orr r2,#0x14               \n"
                     "str r2,[r12,#4]            \n"
                     "mov r2,#1                  \n"
                     "str r2,[r12]               \n"
                     "ldr r1,[r1,#48]            \n"
                     "add r2,r12,#4              \n"
                     "ldmia r1!,{r5-r10}         \n"
                     "stmia r2!,{r5-r10}         \n"
                     "mov r2,#7                  \n"
                     "str r2,[r12]               \n"
                     "ldr r3,[r12,#8]            \n"
                     "orrs r3,#1                 \n"
                     "str r3,[r12,#8]            \n"
                     "dsb                        \n"
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
        ".global __first_task_start     \n"
        "__first_task_start:            \n"
        "svc 0xF5                       \n" 
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_make              \n"
        "task_make:                     \n"
        "svc 0xF4                       \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_unblock           \n"
        "task_unblock:                  \n"
        "svc 0x2                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global task_delay_cancel      \n"
        "task_delay_cancel:             \n"
        "svc 0x3                        \n"
        "bx lr                          \n"
        
        );

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
        ".global mutex_take             \n"
        "mutex_take:                    \n"
        "svc 0xD                        \n"
        "bx lr                          \n"
        );

__asm__(
        ".thumb_func                    \n"
        ".global mutex_put              \n"
        "mutex_put:                     \n"
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
        ".global msgq_aqcuire           \n"
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
        "msgq_release_dealocate:        \n"
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
static inline __attribute__((always_inline)) uint16_t __ldrexh(volatile uint16_t* addr){
    uint32_t res;
    __asm__ volatile ("ldrexh %0, %1" : "=r"(res): "Q"(*addr): "memory");
    return res;
}

static inline __attribute__((always_inline)) uint32_t __strexh(uint16_t val,volatile uint16_t * addr){
    uint32_t res;
    __asm__ volatile ("strexh %0, %2, %1" : "=&r" (res), "=Q" (*addr) : "r" (val));
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

static inline __attribute__((always_inline)) uint32_t __modify_basepri(uint32_t new_basepri){
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

static inline __attribute__((always_inline)) uint32_t __get_ipsr(){
    uint32_t res;
    __asm__ volatile("mrs %0, ipsr":"=r"(res));
    return res;
}

#endif

#endif
