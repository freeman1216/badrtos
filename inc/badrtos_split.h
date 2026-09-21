/**
* @file badrtos_split.h
* @brief Header only rtos implementation

Dont use this file, intended for development

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
*  Public macro to get error from taskhandle
*  
*  @param[in] bad_task_handle_t task handle
*  
*  @retval BAD_RTOS_STATUS_OK task handle valid
*  @retval BAD_RTOS_STATUS_BAD_PARAMETERS on bad configurations
*  @retval BAD_RTOS_STATUS_ALLOC_FAIL on allocation falure
* 
#define BAD_TASK_HANDLE_INVALID_GET_ERROR(handle)

#define TASK_HANDLE_INVALID_GET_ERROR(handle) ((handle) >> 16)
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
* @param[in] u32 delay in ticks 
* @param[in] cbptr cb callback to run 
* @param[in] void* args arguments for the callback
*
* @retval BAD_RTOS_STATUS_OK delay time ran out
* @retval BAD_RTOS_STATUS_WOKEN the task was woken by another task or isr
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT the function was called by an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
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
* @retval u32 previous lock state
*
* extern u32 sched_lock();

**
* \b sched_unlock 
*
* Public svc call (svc 0xF1) that calls internal function __sched_unlock
* Enables scheduler operation, restarts context switching
*
* @param[in] u32 previous lock state
*
* extern void sched_unlock(u32 key);

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
* extern bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, u32 block_size, u32 size_in_bytes);

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
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Mutex successfully taken
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
* @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
* @retval BAD_RTOS_STATUS_WRONG_CONTEXT function was called from an isr
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,u32 delay);

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
* @param[in] u16 Value to initialise semaphore counter with
*
* @retval BAD_RTOS_STATUS_OK semaphore successfully initialised
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS semaphore ptr is null 
*
* extern bad_rtos_status_t sem_init(bad_sem_t *sem,u32 reset_value);

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
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Semaphore successfully taken
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS mutex ptr is null
* @retval BAD_RTOS_STATUS_NOT_INITIALISED init flag is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK take failed without blocking the caller
* @retval BAD_RTOS_STATUS_SCHED_LOCKED sched locked
*
* extern bad_rtos_status_t sem_take(bad_sem_t *sem,u32 delay);

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
* @param[in] u32 capacity Number of messages the queue can hold (MUST be a power of 2)
*
* @retval BAD_RTOS_STATUS_OK Queue successfully allocated and bound to current task
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL or capacity is not a power of 2
* @retval BAD_RTOS_STATUS_NOT_OWNER Queue is already owned by another task
* @retval BAD_RTOS_STATUS_ALREADY_BOUND The current task already owns a message queue
*
* extern bad_rtos_status_t msgq_acquire_allocate(bad_msgq_t *q, u32 capacity);

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
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Message successfully pulled
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q or writeback ptr is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_NOT_OWNER Current task is not the owner of this queue
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay is -1 and queue is empty
* @retval BAD_RTOS_STATUS_TIMEOUT blocked for N ticks without receiving a message
*
* extern bad_rtos_status_t msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback, u32 delay);

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
* @param[in] u32 signal The 32-bit signalID of the message
* @param[in] void* args Ptr to message arguments or payload
* @param[in] u32 delay ticks 0 = block, -1 = dont block, N = block for N ticks
*
* @retval BAD_RTOS_STATUS_OK Message successfully posted
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS q is NULL
* @retval BAD_RTOS_STATUS_NOT_INITIALISED Queue capacity is 0
* @retval BAD_RTOS_STATUS_WOULD_BLOCK delay is -1 and queue is full
* @retval BAD_RTOS_STATUS_TIMEOUT blocked for N ticks without space freeing up
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
* @param[in] u32 flag Bitmask representing the specific event(s) to set
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
* @param[in] u32 flag Bitmask representing the specific event(s) to set
*
* @retval BAD_RTOS_STATUS_OK Flag successfully set (barrier may or may not have fired)
* @retval BAD_RTOS_STATUS_BAD_PARAMETERS event_barrier is NULL, flag is 0, or flag contains invalid bits
* @retval BAD_RTOS_STATUS_NOT_INITIALISED barrier count is 0 (unprimed)
* @retval BAD_RTOS_STATUS_FIRED the barrier has already fired
*
* extern bad_rtos_status_t event_barrier_fire(bad_event_barrier_t *event_barrier, u32 flag);

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
* #define DEFINE_GENERIC_REGION(address, size, mair_idx, share, xn, ap)

**
* /b DEFINE_DMA_BUFF_REGION
* #define DEFINE_DMA_BUFF_REGION(name, address, size) 

**
* /b DEFINE_PERIPH_ACCESS_REGION
* #define DEFINE_PERIPH_ACCESS_REGION(name, address, size) 

**
* /b DEFINE_STATIC_STACK_REGION
* #define DEFINE_STATIC_STACK_REGION(name, address, size) 

**
* /b END_TASK_MPU_REGIONS
* #define END_TASK_MPU_REGIONS(name)

**
* /b DEFINE_DMA_BUFF
*
 * #define DEFINE_DMA_BUFF(name,size)

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
#ifndef BAD_RTOS_H
#define BAD_RTOS_H

#include <stdint.h>

#define KB (1024)

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

#define BAD_RTOS_FLASH_RO_ADDR (0x08000000) //start of RO region
#define BAD_RTOS_FLASH_RO_SIZE (512 * KB)//used to setup mpu RO region
#define BAD_RTOS_RAM_ADDR (0x20000000) //start of RAM 
#define BAD_RTOS_RAM_SIZE  (128 * KB)//used to setup mpu RAM region

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
    BAD_RTOS_STATUS_ALREADY_BOUND,
    BAD_RTOS_STATUS_SCHED_LOCKED,
    BAD_RTOS_STATUS_FIRED,
    BAD_RTOS_STATUS_IN_USE
} bad_rtos_status_t;

typedef enum 
{
    BAD_RTOS_MISC_RUNNING,
    BAD_RTOS_MISC_READYQ_MEMBER,
    BAD_RTOS_MISC_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER,
    BAD_RTOS_MISC_EVENT_BARRIER_BLOCKEDQ_MEMBER
} bad_rtos_misc_t;

typedef enum
{
    BAD_RTOS_MISC_NOT_DELAYED,
    BAD_RTOS_MISC_DELAYQ_MEMBER
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
    uint8_t *addr;
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

// main fat struct of the program
typedef struct bad_tcb bad_tcb_t;
struct bad_tcb
{
    // stack pointer, doesnt really reflect the actual one when running, actual one is + 32
    // (due to registers stacked by hardware), used only to save it for a context switch     
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
#if defined (BAD_RTOS_USE_MPU)
    bad_mpu_region_t regions[4];
#endif
    bad_rtos_misc_t misc;
    bad_rtos_delayq_misc_t delayq_misc;
    u16 generation; //for handles
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
    u8 * volatile next;
    u8 *mem;
    volatile u32 curr;
    u32 size_in_bytes;
    u32 block_size;
}bad_pool_t;

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
    volatile u16 head;
    volatile u16 tail;
    bad_msg_block_t *msgs;
    u8 dynamic;
} bad_msgq_t;

_Static_assert(__builtin_offsetof(bad_msgq_t,head) % 4 == 0, "Message queue head member should have alignment of 4 for atomic update logic to work");
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

//Macro for static stack definition
#ifdef BAD_RTOS_USE_MPU
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert(size % 32 == 0,"Stack sizes must be multiples of 32");\
_Static_assert(size >= 128,"Stacks must be at least 128 bytes to accomodate exception stacked registers and stack cannary");\
static u8 task_name##_stack[size] __attribute__((section(".static_stacks")));
#else 
#define TASK_STATIC_STACK(task_name,size)\
_Static_assert(size % 8 == 0,"Stack sizes must be multiples of 8");\
_Static_assert(size >= 64,"Stacks must be at least 64 bytes to accomodate exception stacked registers");\
static u8 task_name##_stack[size] __attribute__((section(".static_stacks")));
#endif

// PUBLIC API**********************************************
#define BAD_TASK_HANDLE_IS_VALID(handle) ({ handle.idx != 0xFFFF; })

#define BAD_TASK_HANDLE_GET_ERROR(handle) ({\
BAD_TASK_HANDLE_IS_VALID(handle) ? BAD_RTOS_STATUS_OK : handle.gen; \
})

extern bad_task_handle_t task_make(bad_task_descr_t *descr);
extern bad_rtos_status_t task_delay(u32 delay, cbptr cb, void *args );
extern bad_rtos_status_t task_block();
extern bad_rtos_status_t task_unblock(bad_task_handle_t task);
extern bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t task);
extern bad_rtos_status_t task_yield();
extern bad_rtos_status_t task_finish();
extern bad_rtos_status_t task_delay_cancel(bad_task_handle_t task);
extern bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t task);
extern u32 sched_lock();
extern void sched_unlock(u32 key);
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
extern bad_rtos_status_t mutex_init(bad_mutex_t *mut);
extern bad_rtos_status_t mutex_take(bad_mutex_t *mut,u32 delay);
extern bad_rtos_status_t mutex_put(bad_mutex_t *mut);
extern bad_rtos_status_t mutex_delete(bad_mutex_t *mut);
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
extern bad_rtos_status_t sem_init(bad_sem_t *sem,u32 reset_value);
extern bad_rtos_status_t sem_take(bad_sem_t *sem,u32 delay);
extern bad_rtos_status_t sem_put(bad_sem_t *sem);
extern bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem);
extern bad_rtos_status_t sem_delete(bad_sem_t *sem);
#endif

#ifdef BAD_RTOS_USE_MSGQ
#define MSGQ_STATIC_INIT(name,size)\
_Static_assert((size & (size - 1)) == 0, "queue size must be a power of 2"); \
bad_msg_block_t name##_blocks [size];\
bad_msgq_t name = {.capacity_mask = size - 1,.msgs = name##_blocks};

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
#define EVENT_BARRIER_FLAGS_ARE_VALID(flags) (!!((flags) & EVENT_BARRIER_FLAGS_VALID_MASK))
extern bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, u32 count);
extern u32 event_barrier_wait(bad_event_barrier_t *event_barrier, u32 delay);
extern bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier, u32 flag);
extern bad_rtos_status_t event_barrier_fire(bad_event_barrier_t *event_barrier, u32 flag);
extern bad_rtos_status_t event_barrier_delete(bad_event_barrier_t *event_barrier);
#endif

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
    volatile u32 ticks;
    bad_tcb_t *curr;
    bad_tcb_t *next;
    bad_link_node_t delayq;
    u8 is_running;
    u8 is_unlocked;
    u32 ready_bmask;
    bad_link_node_t readyq[BAD_RTOS_PRIO_COUNT];
    bad_link_node_t blockedq;
    bad_isr_q_t isrq;
} bad_kernel_cb_t;

typedef struct bitmask_slab_cb
{
    u32 free_bitmask;
    bad_tcb_t node_arr[BAD_RTOS_MAX_TASKS];
} tcb_bitmask_slab_t;

typedef enum 
{
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

static u8  __attribute__((aligned(_Alignof(bad_isr_op_obj_t)))) gpool_mem[BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES];
static bad_pool_t gpool;
#ifdef BAD_RTOS_USE_KHEAP

typedef struct 
{
    u8* heap;
    u32 heads_bmask;
    u32 max_order;
    u32 min_order;
    bad_link_node_t* free_list;
    u32* bmask;
} bad_buddy_t;
#define BUDDY_BITMASK_SIZE(max_order,min_order)\
(((1 << (max_order - min_order)) - 1) + 31) >> 5 // bits required = (2 ^ max_order - min_order) - 1, to get the words divide by 32 and round up 

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
#define KHEAP_SIZE 1 << KMAX_ORDER
#define KFREE_LIST_SIZE (KMAX_ORDER-KMIN_ORDER + 1)

static u8 __attribute__((section(".kheap"))) kheap[KHEAP_SIZE];
static bad_buddy_t __attribute__((section(".kernel_bss")))kernel_buddy;
static bad_link_node_t __attribute__((section(".kernel_bss"))) kfreelist[KFREE_LIST_SIZE];
static u32 __attribute__((section(".kernel_bss"))) kbitmask[BUDDY_BITMASK_SIZE(KMAX_ORDER, KMIN_ORDER)]; 
#endif

#define IDLE_TASK_PRIO BAD_RTOS_PRIO_COUNT - 1
#define IDLE_TASK_STACK_SIZE 128

TASK_STATIC_STACK(idle, IDLE_TASK_STACK_SIZE)
// Prototypes for asm helpers, for implementation look right above svc_c function, or grep for "ASM stuff"
extern void idle_task(void *);
extern void __first_task_start();

#ifdef BAD_RTOS_USE_SEMAPHORE
extern bad_rtos_status_t __svc_sem_take(bad_sem_t *sem,u32 delay);
extern bad_rtos_status_t __svc_sem_put(bad_sem_t *sem);
#endif

static inline u32 __attribute__((always_inline)) __get_ipsr();
static inline u32 __attribute__((always_inline)) __modify_basepri(u32 basepri);
static inline void __attribute__((always_inline)) __restore_basepri(u32 basepri);
static inline u32 __attribute__((always_inline)) __ldrex(volatile u32* addr);
static inline u32  __attribute__((always_inline)) __strex(u32 val,volatile u32* addr);
static inline u16 __attribute__((always_inline)) __ldrexh(volatile u16* addr);
static inline u32  __attribute__((always_inline)) __strexh(u16 val,volatile u16* addr);
static inline void  __attribute__((always_inline)) __clrex();
static inline void __attribute__((always_inline)) __set_control(u32 control);
static inline u32 __attribute__((always_inline)) __get_control();
static inline void __attribute__((always_inline)) __dmb();
static inline void __attribute__((always_inline)) __dsb();
static inline void __attribute__((always_inline)) __isb();

#define BAD_OPT_BARRIER __asm__ volatile("":::"memory")
#define BAD_RTOS_STATIC static 

#define __TASK_HANDLE_INVALID_HANDLE(error) (bad_task_handle_t){.gen = error, .idx = 0xFFFF}
#define __TASK_HANDLE_IS_VALID(tcb,handle) ({handle.idx && tcb && tcb->generation == handle.gen;})

#define BAD_CONTAINER_OF(ptr, type, member) ({ \
_Static_assert(__builtin_types_compatible_p(typeof(*(ptr)), typeof(((type *)0)->member)), \
"Pointer type mismatch in container_of"); \
((type *)( (char *)(ptr) - __builtin_offsetof(type, member) ));\
})

//Linker script symbols
extern u8 __kernel_bss;
extern u8 __ekernel_bss;

extern u8 __kernel_data;
extern u8 __ekernel_data;
extern u8 __rkernel_data;

extern u8 __static_stacks;

extern u8 __heap;

extern u8 __dma_buffs;

#ifdef BAD_PLATFORM_F411
#include "badrtos_platform_armv7.h"
#endif

#ifdef BAD_PLATFORM_H562
#include "badrtos_platform_armv8.h"
#endif

// Memory helpers
#ifdef BAD_RTOS_USE_KHEAP

void  __buddy_init(bad_buddy_t *cb,
                   u8 *heap, 
                   bad_link_node_t *freelist,
                   u32 min_order, 
                   u32 max_order, 
                   u32 *bmask)
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
    
    for (u32 i = 1; i < max_order - min_order + 1; i++)
    {
        cb->free_list[i].next = &cb->free_list[i];
        cb->free_list[i].prev = &cb->free_list[i];
    }
    
    cb->heads_bmask = 1;
}

static void* __buddy_alloc(bad_buddy_t *cb,u32 order)
{
    if(order > cb->max_order )
    {
        return 0;
    }
    
    u32 idx = cb->max_order  - order;
    u32 order_mask = (1 << (idx + 1)) - 1;
    
    u32 picked_idx = 31 - __builtin_clz(cb->heads_bmask & order_mask);
    
    if(picked_idx == UINT32_MAX)
    {
        return 0;
    }
    
    u32 splits = idx - picked_idx;
    u8 *block_for_split = (u8 *)cb->free_list[picked_idx].next;
    
    cb->free_list[picked_idx].next = cb->free_list[picked_idx].next->next;
    cb->free_list[picked_idx].next->prev = &cb->free_list[picked_idx];
    cb->heads_bmask ^= (u32)(&cb->free_list[idx] == cb->free_list[idx].next) << picked_idx;
    
    u32 splited_block_size = 1 << (cb->max_order - picked_idx-1);
    
    bad_link_node_t *unused_block;
    u32 bmaskidx, bmask_word, bmask_bit,offset_from_base;
    
    if(picked_idx)
    {
        offset_from_base =  block_for_split - cb->heap;
        bmaskidx = ((1 << (picked_idx - 1)) - 1) + ((offset_from_base) >> (cb->max_order - picked_idx + 1));
        bmask_word = bmaskidx >> 5;
        bmask_bit = bmaskidx & 31;
        cb->bmask[bmask_word] ^= 1 << bmask_bit; 
    }
    
    for(u32 i = 0; i < splits;i++)
    {
        unused_block = (bad_link_node_t *)(block_for_split + splited_block_size);
        unused_block->next = cb->free_list[picked_idx + 1].next;
        cb->free_list[picked_idx+1].next = unused_block;
        unused_block->prev = &cb->free_list[picked_idx+1];
        unused_block->next->prev = unused_block;
        
        offset_from_base = (u8 *)unused_block - cb->heap;
        bmaskidx = ((1 << (picked_idx)) - 1) + ((offset_from_base) >> (cb->max_order - picked_idx));
        bmask_word = bmaskidx >> 5;
        bmask_bit = bmaskidx & 31;
        cb->bmask[bmask_word] ^= 1 << bmask_bit;        
        picked_idx++;
        cb->heads_bmask |= (1 << picked_idx);
        splited_block_size >>= 1;
    }
    
    return block_for_split;
}

static void __buddy_free(bad_buddy_t *cb,void *block,u32 order )
{
    
    if(order > cb->max_order)
    {
        return;
    }
    
    u32 curr_order = order; 
    void *curr_block = block;
    u32 idx = 0;
    
    while((idx = cb->max_order - curr_order))
    {
        u32 buddy_bitmask = 1ULL << curr_order;
        
        u32 offset_from_base = (u8 *)curr_block - cb->heap;
        
        u32 bmaskidx =  ((1 << (idx - 1)) - 1) + ((offset_from_base) >> (curr_order + 1));
        u32 bmask_word = bmaskidx >> 5;
        u32 bmask_bit = bmaskidx & 31;
        
        cb->bmask[bmask_word] ^= 1 << bmask_bit;
        
        if(cb->bmask[bmask_word] & 1 << bmask_bit)
        {
            break;
        }
        
        u32 buddy_offset = offset_from_base ^ buddy_bitmask;
        u32 parent_offset = offset_from_base &(~buddy_bitmask);
        void *buddy_addr = (void*)(cb->heap + buddy_offset);
        void *parent_addr = (void*)(cb->heap + parent_offset); 
        
        
        bad_link_node_t *buddy = (bad_link_node_t *)buddy_addr;
        buddy->prev->next = buddy->next;
        buddy->next->prev = buddy->prev;
        buddy->prev = 0;
        buddy->next = 0;
        
        cb->heads_bmask ^= (u32)(&cb->free_list[idx] == cb->free_list[idx].next) << idx;
        
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

BAD_RTOS_STATIC void* __kernel_alloc(u32 size)
{
    u32 closest_order;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    
    if(closest_order < kernel_buddy.min_order)
    {
        closest_order = kernel_buddy.min_order;
    }
    
    return __buddy_alloc(&kernel_buddy,closest_order );
}

BAD_RTOS_STATIC void __kernel_free(void *block,u32 size)
{
    u32 closest_order;
    closest_order = 32 -__builtin_clz(size) - !(size & (size-1));
    
    if(closest_order < kernel_buddy.min_order)
    {
        closest_order = kernel_buddy.min_order;
    }
    
    __buddy_free(&kernel_buddy,block,closest_order );
}
#endif

BAD_RTOS_STATIC void __tcb_queue_slab_init()
{
#if BAD_RTOS_MAX_TASKS < 32
    tcbslab.free_bitmask = (1UL << (BAD_RTOS_MAX_TASKS)) - 1;
#else
    tcbslab.free_bitmask = UINT32_MAX;
#endif
}

BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_alloc()
{
    if(tcbslab.free_bitmask == 0)
    {
        return 0; 
    }
    
    u8 block_idx = __builtin_ctz(tcbslab.free_bitmask);
    tcbslab.free_bitmask &= ~(1UL << block_idx);
    
    return tcbslab.node_arr + block_idx;
}

BAD_RTOS_STATIC u8 __tcb_slab_get_idx_from_ptr(bad_tcb_t *block)
{
    if(tcbslab.node_arr > block || tcbslab.node_arr + BAD_RTOS_MAX_TASKS < block)
    {
        return 0xFF;
    }
    
    return block - tcbslab.node_arr;
}

BAD_RTOS_STATIC bad_tcb_t *__tcb_slab_get_ptr_from_idx(u8 idx)
{
    if(idx >= BAD_RTOS_MAX_TASKS)
    {
        return 0;
    }
    
    return tcbslab.node_arr+idx;
}

BAD_RTOS_STATIC void __tcb_slab_free(bad_tcb_t *tcb)
{
    u8 block_idx = __tcb_slab_get_idx_from_ptr(tcb); 
    
    if(block_idx >= BAD_RTOS_MAX_TASKS)
    {
        return;
    }
    
    tcbslab.free_bitmask |= (1ULL << block_idx); 
}

BAD_RTOS_STATIC void *__obj_list_pull_atomic(volatile void* list)
{
    u32 *head;
    
    do
    {
        head = (u32 *)__ldrex(list);
        if(!head){
            __clrex();
            return 0;
        }
    }while(__strex(*head, (volatile u32 *)list));
    return head;   
}

BAD_RTOS_STATIC void __obj_list_push_atomic(volatile void *list,void *obj)
{
    u32 *new_head = obj;
    
    do
    {
        *new_head = __ldrex(list);
    }
    while(__strex((u32)new_head, (volatile u32 *)list));
}

bad_rtos_status_t pool_init(bad_pool_t *pool, void *mem, u32 block_size, u32 size_in_bytes){
    if(!pool || !mem || !block_size ||!size_in_bytes || size_in_bytes % block_size)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    pool->mem = mem;
    pool->block_size = block_size;
    BAD_OPT_BARRIER;
    pool->size_in_bytes = size_in_bytes;
    return BAD_RTOS_STATUS_OK;
}

void* pool_alloc(bad_pool_t *pool)
{
    void *res =__obj_list_pull_atomic(&pool->next);
    if(res)
    {
        return res;
    }
    
    u32 curr;
    do
    {
        curr = __ldrex(&pool->curr);
        
        if(curr >= pool->size_in_bytes)
        {
            return 0;
        }
        
    }
    while(__strex(curr+pool->block_size,&pool->curr));
    
    return pool->mem + curr;
}

void pool_free(bad_pool_t *pool,void *obj)
{
    u8 *cmp_ptr = obj;
    
    if(pool->mem > cmp_ptr || pool->mem + pool->size_in_bytes <= cmp_ptr)
    {
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

BAD_RTOS_STATIC void __prio_list_enqueue(bad_link_node_t *q,bad_tcb_t *tcb, bad_rtos_misc_t target)
{
    bad_link_node_t *traverse = q->next;
    bad_link_node_t *prev = q;
    
    bad_tcb_t *tcb_to_compare = BAD_CONTAINER_OF(traverse,bad_tcb_t,qnode);
    
    while (traverse && tcb_to_compare->raised_priority <= tcb->raised_priority)
    {
        prev = traverse;
        traverse = traverse->next;
        tcb_to_compare = BAD_CONTAINER_OF(traverse,bad_tcb_t,qnode);
    }
    
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode; 
    tcb_qnode_ptr->next = traverse;
    tcb_qnode_ptr->prev = prev;
    tcb->misc = target;
    
    if(traverse)
    {
        traverse->prev = tcb_qnode_ptr;
    }
    
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr;
}

BAD_RTOS_STATIC void __readyq_enqueue(bad_tcb_t *tcb)
{
    bad_link_node_t *head =  &kernel_cb.readyq[tcb->raised_priority];
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode;
    
    tcb_qnode_ptr->prev = head->prev;
    tcb_qnode_ptr->next = head;
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr;
    
    head->prev = tcb_qnode_ptr;
    kernel_cb.ready_bmask |= 1 << tcb->raised_priority;
    tcb->misc = BAD_RTOS_MISC_READYQ_MEMBER;
}

BAD_RTOS_STATIC u32 __get_top_ready_prio()
{
    return __builtin_ctz(kernel_cb.ready_bmask);
}

BAD_RTOS_STATIC bad_tcb_t *__readyq_dequeue_head()
{
    u32 top = __get_top_ready_prio();
    bad_link_node_t *tcb_qnode_ptr = kernel_cb.readyq[top].next;
    bad_tcb_t *tcb = BAD_CONTAINER_OF(tcb_qnode_ptr, bad_tcb_t, qnode);
    
    tcb_qnode_ptr->next->prev = tcb_qnode_ptr->prev;
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr->next;
    tcb_qnode_ptr->next = 0;
    tcb_qnode_ptr->prev = 0;
    
    kernel_cb.ready_bmask ^= (kernel_cb.readyq[top].next ==  &kernel_cb.readyq[top]) << top;
    return tcb;
}

BAD_RTOS_STATIC void __delayq_enqueue(bad_tcb_t *tcb, u32 absolute)
{
    bad_link_node_t *traverse = kernel_cb.delayq.next;
    bad_link_node_t *prev = &kernel_cb.delayq;
    bad_tcb_t *traverse_tcb = BAD_CONTAINER_OF(traverse,bad_tcb_t,delaynode);
    u32 compound = 0;
    
    while (traverse && (compound+=traverse_tcb->counter) <= absolute)
    {
        prev = traverse;
        traverse = traverse->next;
        traverse_tcb = BAD_CONTAINER_OF(traverse,bad_tcb_t,delaynode);
    }
    
    bad_link_node_t *tcb_delaynode_ptr = &tcb->delaynode;
    
    tcb_delaynode_ptr->next = traverse;
    tcb_delaynode_ptr->prev = prev;
    tcb->delayq_misc = BAD_RTOS_MISC_DELAYQ_MEMBER;
    
    if(traverse)
    {
        traverse->prev = tcb_delaynode_ptr;
        compound -= traverse_tcb->counter;
        traverse_tcb->counter -= absolute - compound;
    }
    
    tcb->counter = absolute - compound;
    tcb_delaynode_ptr->prev->next = tcb_delaynode_ptr;
}

BAD_RTOS_STATIC bad_rtos_status_t __delayq_dequeue(bad_tcb_t *tcb)
{
    if(tcb->delayq_misc == BAD_RTOS_MISC_NOT_DELAYED)
    {
        return BAD_RTOS_STATUS_WRONG_Q;
    }
    
    bad_link_node_t *tcb_delaynode_ptr = &tcb->delaynode;
    tcb_delaynode_ptr->prev->next = tcb_delaynode_ptr->next;
    
    if(tcb_delaynode_ptr->next)
    {
        bad_tcb_t *next_tcb = BAD_CONTAINER_OF(tcb_delaynode_ptr->next,bad_tcb_t,delaynode);
        next_tcb->counter+=tcb->counter;
        tcb_delaynode_ptr->next->prev = tcb_delaynode_ptr->prev;
    }
    
    tcb_delaynode_ptr->next = 0;
    tcb_delaynode_ptr->prev = 0;
    tcb->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_tcb_t* __prio_list_dequeue_head(bad_link_node_t *q)
{
    bad_link_node_t *head = q->next;
    
    if(!head)
    {
        return 0;
    }
    
    bad_link_node_t *new_head = head->next; 
    q->next = new_head;
    
    if(new_head)
    {
        new_head->prev = q;
    }
    
    return BAD_CONTAINER_OF(head,bad_tcb_t,qnode);
}

BAD_RTOS_STATIC bad_tcb_t* __delayq_dequeue_head()
{
    bad_link_node_t *head = kernel_cb.delayq.next;
    
    if(!head)
    {
        return 0;
    }
    bad_link_node_t *new_head = head->next; 
    kernel_cb.delayq.next = new_head;
    
    if(new_head)
    {
        new_head->prev = &kernel_cb.delayq;
    }
    
    bad_tcb_t *head_tcb = BAD_CONTAINER_OF(head,bad_tcb_t,delaynode);
    head_tcb->delayq_misc = BAD_RTOS_MISC_NOT_DELAYED; 
    
    return head_tcb;
}

BAD_RTOS_STATIC void __enqueue_head(bad_link_node_t *q, bad_tcb_t *tcb, bad_rtos_misc_t target)
{
    bad_link_node_t * old_head = q->next;
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode;
    
    if(old_head)
    {
        old_head->prev = tcb_qnode_ptr;
    }
    
    tcb_qnode_ptr->next = old_head;
    tcb_qnode_ptr->prev = q;
    tcb->misc = target;
    q->next = tcb_qnode_ptr;
}

BAD_RTOS_STATIC bad_rtos_status_t __remove_entry(bad_tcb_t *tcb,bad_rtos_misc_t target)
{
    if(tcb->misc != target)
    {
        return BAD_RTOS_STATUS_WRONG_Q;
    }
    
    bad_link_node_t *tcb_qnode_ptr = &tcb->qnode;
    tcb_qnode_ptr->prev->next = tcb_qnode_ptr->next;
    
    if(tcb_qnode_ptr->next)
    {
        tcb_qnode_ptr->next->prev = tcb_qnode_ptr->prev;
    }
    
    return BAD_RTOS_STATUS_OK;
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
        {
            return 0;
        }
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
    {
        return 0; //not possible in the current usecase but nice to have
    }
    
    __isr_q_push(q,&q->stub);
    next = tail->next;
    
    if(!next)
    {
        return 0;//not possible in the current usecase but nice to have
    }
    
    q->tail = next;
    return tail;
}

BAD_RTOS_STATIC bad_rtos_status_t __kernel_notify(bad_isr_op_t op,void *arg)
{
    bad_isr_op_obj_t *message = gpool_alloc();
    
    if(!message)
    {
        return BAD_RTOS_STATUS_ALLOC_FAIL;
    }
    message->op_kind = op;
    message->arg = arg;
    __dmb();
    __isr_q_push(&kernel_cb.isrq,message);
    __scb_trigger_pendsv();
    
    return BAD_RTOS_STATUS_OK; 
}

BAD_RTOS_STATIC void __sched_update(bad_tcb_t *tcb)
{
    kernel_cb.next = tcb;
    tcb->misc = BAD_RTOS_MISC_RUNNING;
}

BAD_RTOS_STATIC void __sched_try_update()
{
    u32 top_ready_prio = __get_top_ready_prio();
    
    if(top_ready_prio < kernel_cb.curr->raised_priority && kernel_cb.is_unlocked)
    {
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}

BAD_RTOS_STATIC void __sched_try_preempt(bad_tcb_t *tcb)
{
    if(tcb->raised_priority < kernel_cb.curr->raised_priority && kernel_cb.is_unlocked)
    {
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(tcb);
    }
    else
    {
        __readyq_enqueue(tcb);
    }
}

static void __attribute__((used)) __handle_systick_event(bad_systick_status_t status)
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
        while(kernel_cb.delayq.next && !BAD_CONTAINER_OF(kernel_cb.delayq.next, bad_tcb_t, delaynode)->counter);
        
    }
    
    if (status & BAD_SYSTICK_TIMEFRAME_PENDING)
    {
        kernel_cb.curr->counter = kernel_cb.curr->ticks_to_change;
    }    
    
    u32 top_ready_prio = __get_top_ready_prio(); 
    
    if(kernel_cb.ready_bmask && kernel_cb.is_unlocked && 
       top_ready_prio + (status == BAD_SYSTICK_DELAY_WAKE_PENDING)
       <= kernel_cb.curr->raised_priority )
    {
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
    }
}

BAD_RTOS_STATIC u32 * __init_stack(taskptr task, u32 *stacktop,void *args)
{
    *--stacktop = 0x01000000UL;     // xPSR (Thumb bit set)
    *--stacktop = (u32)task|0x1;   // PC
    *--stacktop = 0x0;     
    *--stacktop = 0x12121212UL;     // R12
    *--stacktop = 0x03030303UL;     // R3
    *--stacktop = 0x02020202UL;     // R2
    *--stacktop = 0x01010101UL;     // R1
    *--stacktop = (u32)args;     // R0 (parameter, optional)
    *--stacktop = 0xFFFFFFFDUL;  //lr pushed to track fpu state (thread mode + PSP + No FPU)
    for(u8 i = 0;i < 8;i++)
    {
        *--stacktop = 0xDEADBEEFUL;
    }
    return stacktop;
}

//Core isr api implementations
bad_rtos_status_t task_unblock_from_isr(bad_task_handle_t handle)
{
    if(!__get_ipsr())
    {
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    if(!__TASK_HANDLE_IS_VALID(tcb,handle))
    {
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    return __kernel_notify(BAD_ISR_OP_TASK_UNBLOCK,(void*)handle.val);
}

bad_rtos_status_t task_delay_cancel_from_isr(bad_task_handle_t handle)
{
    if(!__get_ipsr())
    {
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    if(!__TASK_HANDLE_IS_VALID(tcb,handle))
    {
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    return __kernel_notify(BAD_ISR_OP_TASK_DELAY_CANCEL,(void*)handle.val);
}

//Core api implenetations
BAD_RTOS_STATIC bad_task_handle_t __task_make(bad_task_descr_t *args)
{
    bad_rtos_status_t ret = BAD_RTOS_STATUS_OK;
    bad_tcb_t *new_task = 0;
    
    if(args->stack_size < 64 || args->stack_size % 32 || args->base_priority >= IDLE_TASK_PRIO)
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
            ret = BAD_RTOS_STATUS_ALREADY_BOUND;
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
    new_task->dyn_stack = 0;
    
    if(args->stack)
    {
        new_task->stack = args->stack;
        new_task->dyn_stack = 0;
    }
    else
    {
#if defined(BAD_RTOS_USE_KHEAP)
        new_task->stack = __kernel_alloc(args->stack_size);
        if(!new_task->stack){
            ret = BAD_RTOS_STATUS_ALLOC_FAIL;
            goto err_release_msgq;
        }
        new_task->dyn_stack = 1;
#else
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
    new_task->sp = __init_stack(new_task->entry, stack_top, args->args);
    
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
    
    err_free_stack:
    __kernel_free(new_task->stack,args->stack_size);
    
    err_release_msgq:
#ifdef BAD_RTOS_USE_MSGQ
    if(args->assigned_msgq)
    {
        args->assigned_msgq->owner = 0;
    }
#endif
    
    err_free_tcb:
    __tcb_slab_free(new_task);
    
    exit_error:
    return __TASK_HANDLE_INVALID_HANDLE(ret);
}

BAD_RTOS_STATIC void  __task_block()
{
    __enqueue_head(&kernel_cb.blockedq, kernel_cb.curr,BAD_RTOS_MISC_BLOCKEDQ_MEMBER);
    __sched_update(__readyq_dequeue_head());
}

BAD_RTOS_STATIC bad_rtos_status_t __task_unblock(bad_task_handle_t handle)
{
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    if(!__TASK_HANDLE_IS_VALID(tcb,handle))
    {
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    if(__remove_entry(tcb, BAD_RTOS_MISC_BLOCKEDQ_MEMBER) != BAD_RTOS_STATUS_OK)
    {
        return BAD_RTOS_STATUS_NOT_BLOCKED;
    }
    
    __sched_try_preempt(tcb);
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __task_delay_cancel(bad_task_handle_t handle)
{
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    if(!__TASK_HANDLE_IS_VALID(tcb,handle))
    {
        return BAD_RTOS_STATUS_HANDLE_INVALID;
    }
    
    if(__delayq_dequeue(tcb)!= BAD_RTOS_STATUS_OK)
    {
        return BAD_RTOS_STATUS_NOT_DELAYED;
    }
    
    tcb->cbptr = 0;
    tcb->args = 0;
    *(tcb->sp+9) = BAD_RTOS_STATUS_WOKEN;
    
    __sched_try_preempt(tcb);
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __task_yield()
{
    u32 top_ready_prio = __get_top_ready_prio();
    
    if(top_ready_prio == kernel_cb.curr->raised_priority && kernel_cb.is_unlocked)
    {
        __readyq_enqueue(kernel_cb.curr);
        __sched_update(__readyq_dequeue_head());
        return BAD_RTOS_STATUS_OK;
    }
    else
    {
        return BAD_RTOS_STATUS_CANT_YIELD;
    }
}

BAD_RTOS_STATIC void __task_finish()
{
#ifdef BAD_RTOS_USE_MUTEX
    if(kernel_cb.curr->mutex_count) //trap when task want to finish without releasing mutexes
    {
        __builtin_trap();
        return;
    }
#endif 
    
#ifdef BAD_RTOS_USE_MSGQ
    if(kernel_cb.curr->msgq_owner)
    {
        __builtin_trap();
        return;
    }
#endif
    
#if defined (BAD_RTOS_USE_KHEAP)
    if(kernel_cb.curr->dyn_stack) //free the dynamically allocated stack 
    {
        __kernel_free((void*)kernel_cb.curr->stack,kernel_cb.curr->stack_size);
    }
#endif
    
    __sched_update(__readyq_dequeue_head());
    kernel_cb.curr->generation++;
    __tcb_slab_free(kernel_cb.curr); //free the the tcb used by task
}

BAD_RTOS_STATIC void __task_delay(u32 delay,cbptr cb, void* args)
{
    kernel_cb.curr->cbptr =cb;
    kernel_cb.curr->args = args;
    
    __delayq_enqueue(kernel_cb.curr,delay);
    __sched_update(__readyq_dequeue_head());
}

BAD_RTOS_STATIC void __kernel_start()
{
    if(kernel_cb.is_running)
    {
        __builtin_trap();
    }
    
    kernel_cb.is_running = 1;
    kernel_cb.is_unlocked = 1;
    pool_init(&gpool,gpool_mem,sizeof(bad_isr_op_obj_t),sizeof(bad_isr_op_obj_t) * BAD_RTOS_GLOBAL_POOL_SIZE_IN_BYTES);
    
    __set_control(0x1);
    __restore_basepri(0);
    
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_SCB_LOWEST_PRIO);
    
    kernel_cb.curr = __readyq_dequeue_head();
    
    __asm__ volatile("b __init_second_stage");
}

BAD_RTOS_STATIC u32 __sched_lock()
{
    u32 lock = kernel_cb.is_unlocked;
    kernel_cb.is_unlocked = 0;
    return lock;
}

BAD_RTOS_STATIC void __sched_unlock(u32 key)
{
    kernel_cb.is_unlocked = key;
    __sched_try_update();
}

// Startup code
BAD_RTOS_STATIC void __kernel_sections_init()
{
    u32 *src = (u32 *)&__kernel_bss;
    u32 *end = (u32 *)&__ekernel_bss;
    
    while (src < end)
    {
        *src++ = 0; 
    }
    
    src = (u32 *)&__rkernel_data;
    u32 *dest = (u32 *)&__kernel_data;
    end = (u32 *)&__ekernel_data;
    
    while (dest < end)
    {
        *dest++ = *src++;
    }
}

BAD_RTOS_STATIC void __interrupt_init()
{
    __scb_set_core_interrupt_priority(BAD_SCB_SVC_INTR, BAD_SCB_PRIO0);
    __scb_set_core_interrupt_priority(BAD_SCB_PENDSV_INTR, BAD_SCB_LOWEST_PRIO);
}

BAD_RTOS_STATIC void __readyq_init()
{
    for (u32 i = 0; i < BAD_RTOS_PRIO_COUNT; i++){
        kernel_cb.readyq[i].next = &kernel_cb.readyq[i];
        kernel_cb.readyq[i].prev = &kernel_cb.readyq[i];
    }
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
    __readyq_init();
    __interrupt_init();
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
    
    if(bad_user_init() == BAD_RTOS_STATUS_OK)
        __first_task_start();
    else
        __restore_basepri(0);
}

//Synchro helpers

BAD_RTOS_STATIC bad_tcb_t* __synchro_wake(bad_link_node_t *q,cbptr cb,bad_rtos_status_t status)
{
    bad_tcb_t *tcb = __prio_list_dequeue_head(q);
    if(!tcb)
    {
        return 0;
    }
    
    if(tcb->cbptr == cb)
    {
        __delayq_dequeue(tcb);
        tcb->cbptr = 0;
        tcb->args = 0;
    }
    
    *(tcb->sp+9) = status;
    __sched_try_preempt(tcb);
    return tcb;
}

BAD_RTOS_STATIC void __synchro_wake_all(bad_link_node_t *q,cbptr cb, u32 status)
{
    bad_link_node_t *traverse = q->next;
    bad_tcb_t *traverse_tcb = BAD_CONTAINER_OF(traverse, bad_tcb_t, qnode);
    while(traverse)
    {
        *(traverse_tcb->sp+9) = status;
        
        if(traverse_tcb->cbptr == cb)
        {
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

BAD_RTOS_STATIC  bad_rtos_status_t __synchro_block(bad_link_node_t *q, cbptr cb, u32 delay, bad_rtos_misc_t misc)
{
    if(delay == UINT32_MAX)
    {
        return BAD_RTOS_STATUS_WOULD_BLOCK;
    }
    
    if(delay)
    {
        kernel_cb.curr->args = q; //every synchro obj has blockedq as first element
        kernel_cb.curr->cbptr = cb;
        __delayq_enqueue( kernel_cb.curr, delay);
    }
    
    __prio_list_enqueue(q,kernel_cb.curr, misc);
    __sched_update(__readyq_dequeue_head());
    return BAD_RTOS_STATUS_OK;
}

// Synchro objects api implenetations
#ifdef BAD_RTOS_USE_MSGQ

BAD_RTOS_STATIC void __msgq_timeout_cb(bad_task_handle_t handle ,void *msgq)
{
    (void)msgq;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    __remove_entry(tcb,BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

#ifdef BAD_RTOS_USE_KHEAP

BAD_RTOS_STATIC bad_rtos_status_t __msgq_acquire_allocate(bad_msgq_t *q,u32 capacity)
{
    if(!q || (capacity & (capacity - 1)))
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner)
    {
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    kernel_cb.curr->msgq_owner++;
    q->msgs = __kernel_alloc(capacity);
    q->owner = kernel_cb.curr;
    q->dynamic = 1;
    q->blockedq = (bad_link_node_t){0};
    q->head = q->tail = 0;
    BAD_OPT_BARRIER;
    
    q->capacity_mask = capacity - 1;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __msgq_release_deallocate(bad_msgq_t *q){
    if(!q || !q->dynamic)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner != kernel_cb.curr)
    {
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    kernel_cb.curr->msgq_owner--;
    u32 capacity = q->capacity_mask + 1;
    
    q->capacity_mask = 0;
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
    
    q->owner = kernel_cb.curr;
    kernel_cb.curr->msgq_owner++;
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __msgq_release(bad_msgq_t *q)
{
    if(!q)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(q->owner != kernel_cb.curr)
    {
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    q->owner = 0;
    kernel_cb.curr->msgq_owner--;
    volatile u32 *atomic_update = (volatile u32 *)&q->head; 
    *atomic_update = 0;
    __synchro_wake_all(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t __msgq_pull_msg(bad_msgq_t *q, bad_msg_block_t *writeback,u32 delay)
{
    if(!q || !writeback)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!q->capacity_mask)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    if(q->owner != kernel_cb.curr)
    {
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    if(q->tail == q->head)
    {
        return __synchro_block(&q->blockedq,__msgq_timeout_cb,delay, BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER);
    }
    
    *writeback = *(q->msgs + q->tail);
    BAD_OPT_BARRIER;
    bad_tcb_t *tcb = __synchro_wake(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_OK);
    
    if(tcb)
    {
        u16 next_tail = (q->tail + 1) & q->capacity_mask;
        u16 next_head = (q->head + 1) & q->capacity_mask;
        
        u32 signal = *(tcb->sp + 10);
        void *args = (void *)*(tcb->sp + 11);
        
        bad_msg_block_t *block = q->msgs + q->head;
        block->signal = signal;
        block->args = args;
        BAD_OPT_BARRIER;
        
        volatile u32 *atomic_update = (volatile u32 *)&q->head;
        *atomic_update = next_head | (next_tail << 16);
    }
    else
    {
        q->tail = (q->tail + 1) & q->capacity_mask;
    }
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __msgq_try_wake(bad_msgq_t *q)
{
    bad_tcb_t *tcb = __synchro_wake(&q->blockedq,__msgq_timeout_cb,BAD_RTOS_STATUS_OK);
    
    if(tcb)
    {
        bad_msg_block_t *writeback = (bad_msg_block_t *) *(tcb->sp + 10);
        *writeback = *(q->msgs+q->tail);
        
        BAD_OPT_BARRIER;
        q->tail = (q->tail + 1) & q->capacity_mask;
    }
}

bad_rtos_status_t __msgq_post_msg(bad_msgq_t *q, u32 signal, void *args,u32 delay)
{
    u16 head,next_head;
    
    if(!q)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!q->capacity_mask)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    do
    {
        head = __ldrexh(&q->head);
        next_head = (head+1) & q->capacity_mask;
        
        if(q->tail == next_head)
        {
            __clrex();
            return __synchro_block(&q->blockedq,__msgq_timeout_cb,delay, BAD_RTOS_MISC_MSGQ_BLOCKEDQ_MEMBER);
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
    
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t msgq_post_msg_from_isr(bad_msgq_t *q, u32 signal, void *args)
{
    if(!__get_ipsr())
    {
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    if(!q)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!q->capacity_mask)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    u32 head,next_head;
    do
    {
        head = __ldrexh(&q->head);
        next_head = (head+1) & q->capacity_mask;
        
        if(q->tail == next_head)
        {
            __clrex();
            return BAD_RTOS_STATUS_WOULD_BLOCK;
        }
        
    }
    while(__strexh(next_head, &q->head)); 
    
    bad_msg_block_t* block = q->msgs + head;
    
    block->signal = signal;
    block->args = args;
    
    if(q->tail == head) // we may have preempted the consumer mid block, need to check in pendsv
    {
        return __kernel_notify(BAD_ISR_OP_MSGQ_WAKE,q);
    }
    return BAD_RTOS_STATUS_OK;
    
}

#endif

#ifdef BAD_RTOS_USE_MUTEX
bad_rtos_status_t mutex_init(bad_mutex_t *mut)
{
    if(!mut)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    *mut = (bad_mutex_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __mutex_timeout_cb(bad_task_handle_t handle ,void *mutex)
{
    (void)mutex;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    __remove_entry(tcb,BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

BAD_RTOS_STATIC bad_rtos_status_t __mutex_delete(bad_mutex_t *mut)
{
    if(!mut)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(kernel_cb.curr != mut->owner)
    {
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    kernel_cb.curr->mutex_count--;
    
    if(!kernel_cb.curr->mutex_count)
    {
        kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
    }
    
    __synchro_wake_all(&mut->blockedq,__mutex_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    *mut = (bad_mutex_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __mutex_update_owner_pos(bad_tcb_t *owner)
{
    if(owner->misc == BAD_RTOS_MISC_READYQ_MEMBER)
    {
        __remove_entry(owner,BAD_RTOS_MISC_READYQ_MEMBER);
        __readyq_enqueue(owner);
    }
}

BAD_RTOS_STATIC bad_rtos_status_t __mutex_take(bad_mutex_t *mut, u32 delay)
{
    if(!mut)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!mut->owner)
    {
        mut->owner = kernel_cb.curr;
        kernel_cb.curr->mutex_count++;
        return BAD_RTOS_STATUS_OK;
    }
    
    if(mut->owner == kernel_cb.curr)
    {
        mut->rec_takes++;
        return BAD_RTOS_STATUS_OK;
    }
    
    if(kernel_cb.curr->raised_priority < mut->owner->raised_priority)
    {
        mut->owner->raised_priority = kernel_cb.curr->raised_priority;
        __mutex_update_owner_pos(mut->owner);
    }
    
    return __synchro_block(&mut->blockedq,__mutex_timeout_cb,delay, BAD_RTOS_MISC_MUTEX_BLOCKEDQ_MEMBER);
}   

BAD_RTOS_STATIC bad_rtos_status_t __mutex_put(bad_mutex_t *mut)
{
    if(!mut)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(kernel_cb.curr!= mut->owner)
    {
        return BAD_RTOS_STATUS_NOT_OWNER;
    }
    
    if(mut->rec_takes)
    {
        mut->rec_takes--;
        return BAD_RTOS_STATUS_OK;
    }
    
    mut->owner =  __synchro_wake(&mut->blockedq,__mutex_timeout_cb,BAD_RTOS_STATUS_OK);
    
    if(!--kernel_cb.curr->mutex_count)
    {
        kernel_cb.curr->raised_priority = kernel_cb.curr->base_priority;
    }
    
    if(!mut->owner)
    {
        return BAD_RTOS_STATUS_OK; 
    }
    mut->owner->mutex_count++;    
    
    return BAD_RTOS_STATUS_OK;
}
#endif

#ifdef BAD_RTOS_USE_SEMAPHORE
bad_rtos_status_t sem_init(bad_sem_t *sem, u32 reset_value)
{
    if(!sem)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    sem->blockedq = (bad_link_node_t){0};
    sem->counter = reset_value;
    sem->init_flag = 1;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __sem_timeout_cb(bad_task_handle_t handle ,void *semaphore)
{
    (void)semaphore;
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    __remove_entry(tcb,BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER);
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_delete(bad_sem_t *sem)
{
    if(!sem)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    if(!sem->blockedq.next)
    {
        return BAD_RTOS_STATUS_OK;
    }
    
    __synchro_wake_all(&sem->blockedq,__sem_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    *sem = (bad_sem_t){0};
    
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t sem_take(bad_sem_t *sem,u32 delay)
{
    if(!sem)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    u32 counter;
    do
    {
        counter = __ldrex(&sem->counter);
        
        if(!counter)
        {
            __clrex();
            if(delay == UINT32_MAX)
            {
                return BAD_RTOS_STATUS_WOULD_BLOCK;
            }
            
            return __svc_sem_take(sem,delay);
        }
    }
    while(__strex(counter-1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}

bad_rtos_status_t sem_put(bad_sem_t *sem)
{
    if(!sem)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    u32 counter;
    do
    {
        counter = __ldrex(&sem->counter);
        if(((volatile typeof(sem->blockedq) *)&sem->blockedq)->next)
        {
            __clrex();
            return __svc_sem_put(sem);
        }
    }while(__strex(counter + 1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_put(bad_sem_t *sem)
{
    bad_tcb_t *tcb = __synchro_wake(&sem->blockedq,__sem_timeout_cb,BAD_RTOS_STATUS_OK);
    
    if(tcb)
    {
        return BAD_RTOS_STATUS_OK;
    }
    
    u32 counter;
    do
    {
        counter = __ldrex(&sem->counter);
    }
    while(__strex(counter + 1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __sem_take(bad_sem_t *sem, u32 delay)
{
    if(!sem->counter)
    {
        return __synchro_block(&sem->blockedq, __sem_timeout_cb, delay, BAD_RTOS_MISC_SEM_BLOCKEDQ_MEMBER);
    }
    
    u32 counter;
    do
    {
        counter = __ldrex(&sem->counter);
    }
    while(__strex(counter-1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
} 

bad_rtos_status_t sem_put_from_isr(bad_sem_t *sem)
{
    if(!__get_ipsr())
    {
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    if(!sem)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!sem->init_flag)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    u32 counter;
    do
    {
        counter = __ldrex(&sem->counter);
        
        if(!counter)
        {
            __clrex();
            return __kernel_notify(BAD_ISR_OP_SEM_PUT,sem);
        }
    }
    while(__strex(counter+1, &sem->counter));
    
    return BAD_RTOS_STATUS_OK;
}
#endif

#ifdef BAD_RTOS_USE_EVENT_BARRIER
static void __event_barrier_timeout_cb(bad_task_handle_t handle ,void *event_barrier)
{
    (void)event_barrier;
    
    bad_tcb_t *tcb = __tcb_slab_get_ptr_from_idx(handle.idx);
    
    __remove_entry(tcb,BAD_RTOS_MISC_EVENT_BARRIER_BLOCKEDQ_MEMBER);
    
    *(tcb->sp+9)=BAD_RTOS_STATUS_TIMEOUT;
}

bad_rtos_status_t event_barrier_prime(bad_event_barrier_t *event_barrier, u32 count)
{
    if(!event_barrier|| !count || count >= 32)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(event_barrier->count && event_barrier->count != 32)
    {
        return BAD_RTOS_STATUS_IN_USE;
    }
    
    *event_barrier = (bad_event_barrier_t){0};
    
    BAD_OPT_BARRIER;
    
    event_barrier->count = count;
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC u32 __event_barrier_wait(bad_event_barrier_t *event_barrier,u32 delay)
{
    if(!event_barrier)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!event_barrier->count)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    if(event_barrier->count == 32)
    {
        return BAD_RTOS_STATUS_FIRED;
    }
    
    return __synchro_block(&event_barrier->blockedq,__event_barrier_timeout_cb,delay, BAD_RTOS_MISC_EVENT_BARRIER_BLOCKEDQ_MEMBER);
}

bad_rtos_status_t event_barrier_fire_from_isr(bad_event_barrier_t *event_barrier,u32 flag)
{
    if(!__get_ipsr())
    {
        return BAD_RTOS_STATUS_WRONG_CONTEXT;
    }
    
    if(!event_barrier ||!flag ||flag == EVENT_BARRIER_FLAGS_VALID_MASK)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!event_barrier->count)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    u32 flags,new_flags;
    do
    {
        flags = __ldrex(&event_barrier->flags);
        
        if(event_barrier->count == 32)
        {
            __clrex();
            return BAD_RTOS_STATUS_FIRED;
        }
        
        new_flags = flags | flag;
        
        if(new_flags == flags)
        {
            __clrex();
            return BAD_RTOS_STATUS_OK;
        }
    }while(__strex(new_flags, &event_barrier->flags));
    
    if(__builtin_popcount(new_flags) == event_barrier->count)
    {
        event_barrier->count = 32;
        BAD_OPT_BARRIER;
        
        event_barrier->flags = new_flags;//report correct flags on wakeup
        return __kernel_notify(BAD_ISR_OP_EVENT_BARRIER_WAKE,event_barrier);
    }
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC void __event_barrier_wake(bad_event_barrier_t *event_barrier)
{
    u32 flags = event_barrier->flags;
    
    __synchro_wake_all(&event_barrier->blockedq,__event_barrier_timeout_cb,flags | EVENT_BARRIER_FLAGS_VALID_MASK);
}

BAD_RTOS_STATIC bad_rtos_status_t __event_barrier_fire(bad_event_barrier_t *event_barrier,u32 flag)
{
    if(!event_barrier ||!flag ||flag == EVENT_BARRIER_FLAGS_VALID_MASK)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!event_barrier->count)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED; 
    }    
    
    u32 flags,new_flags;
    do
    {
        flags = __ldrex(&event_barrier->flags);
        
        if(event_barrier->count == 32)
        {
            __clrex();
            return BAD_RTOS_STATUS_FIRED;
        }
        
        new_flags = flags | flag;
        
        if(new_flags == flags)
        {
            __clrex();
            return BAD_RTOS_STATUS_OK;
        }    
    }
    while(__strex(new_flags, &event_barrier->flags));
    
    if(__builtin_popcount(new_flags) == event_barrier->count)
    {
        event_barrier->count = 32; 
        BAD_OPT_BARRIER;
        
        event_barrier->flags = new_flags;//report correct flags on wakeup
        __event_barrier_wake(event_barrier);
    }
    
    return BAD_RTOS_STATUS_OK;
}

BAD_RTOS_STATIC bad_rtos_status_t __event_barrier_delete(bad_event_barrier_t *event_barrier)
{
    if(!event_barrier)
    {
        return BAD_RTOS_STATUS_BAD_PARAMETERS;
    }
    
    if(!event_barrier->count)
    {
        return BAD_RTOS_STATUS_NOT_INITIALISED;
    }
    
    __synchro_wake_all(&event_barrier->blockedq,__event_barrier_timeout_cb,BAD_RTOS_STATUS_DELETED);
    
    *event_barrier = (bad_event_barrier_t){0};
    
    return BAD_RTOS_STATUS_OK;
}
#endif

//ISRS

static void __attribute__((used)) __svc_c(u8 svc, u32* stack)
{
    bad_task_handle_t handle = {0};
    handle.val = stack[0];
    
    switch (svc)
    {
        case 0x2:
        {
            stack[0]=__task_unblock(handle);
        }break;
        
        case 0x3:
        {
            stack[0] =__task_delay_cancel(handle);
        }break;
        
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
            stack[0] = BAD_RTOS_STATUS_OK;
        }break;
        
        case 0x7:
        {
            __task_delay(stack[0], (cbptr) stack[1] ,(void*)stack[2]);
            stack[0] = BAD_RTOS_STATUS_OK;
        }break;
        
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
        
        case 0xF0:
        {
            stack[0] = __sched_lock();
        }break;
        
        case 0xF1:
        {
            __sched_unlock(stack[0]);
        }break;
        
#ifdef BAD_RTOS_USE_KHEAP
        case 0xF2:
        {
            stack[0]=(u32)__kernel_alloc(stack[0]);
        }break;
        
        case 0xF3:
        {
            __kernel_free((void*)stack[0], stack[1]);
        }break;
#endif
        
        case 0xF4:
        {
            stack[0] = __task_make((bad_task_descr_t*)stack[0]).val;
        }break;
        
        case 0xF5:
        {
            __kernel_start();
        }break;
        
        default:
        {
            __builtin_unreachable();
        }
    }
}

static void __attribute__((used)) __pendsv_c()
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
void __attribute__((naked)) BAD_RTOS_PENDSV_HANDLER_NAME()
{
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
                     ".cfi_adjust_cfa_offset 8 \n"
                     ".cfi_rel_offset r7, 0    \n"
                     ".cfi_rel_offset lr, 4    \n"
#ifdef BAD_RTOS_USE_MPU
                     "ldr r12,=%1              \n"
                     "ldr r3,[r12,#8]          \n"
                     "bic r0,r3,#1             \n"
                     "str r0,[r12,#8]          \n"
                     "push {r3,r12}            \n"
                     ".cfi_adjust_cfa_offset 8 \n" 
                     ".cfi_rel_offset r3, 0    \n"
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
                     
                     "pop {r7,lr}              \n"
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

static inline __attribute__((always_inline)) u32 __ldrex(volatile u32* addr)
{
    u32 res;
    __asm__ volatile ("ldrex %0, %1" : "=r"(res): "Q"(*addr): "memory");
    return res;
}

static inline __attribute__((always_inline)) u32 __strex(u32 val,volatile u32 * addr)
{
    u32 res;
    __asm__ volatile ("strex %0, %2, %1" : "=&r" (res), "=Q" (*addr) : "r" (val));
    return res;
}

static inline __attribute__((always_inline)) u16 __ldrexh(volatile u16* addr)
{
    u32 res;
    __asm__ volatile ("ldrexh %0, %1" : "=r"(res): "Q"(*addr): "memory");
    return res;
}

static inline __attribute__((always_inline)) u32 __strexh(u16 val,volatile u16 * addr)
{
    u32 res;
    __asm__ volatile ("strexh %0, %2, %1" : "=&r" (res), "=Q" (*addr) : "r" (val));
    return res;
}

static inline __attribute__((always_inline)) void __clrex()
{
    __asm__ volatile ("clrex":::"memory");
}

static inline __attribute__((always_inline)) void __dmb()
{
    __asm__ volatile ("dmb":::"memory");
}

static inline __attribute__((always_inline)) void __dsb()
{
    __asm__ volatile ("dsb":::"memory");
}

static inline __attribute__((always_inline)) void __isb()
{
    __asm__ volatile ("isb":::"memory");
}

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

static inline __attribute__((always_inline)) u32 __get_ipsr()
{
    u32 res;
    __asm__ volatile("mrs %0, ipsr":"=r"(res));
    return res;
}

#endif

#endif
