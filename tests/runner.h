/* date = September 23rd 2026 10:47 pm */
#ifndef RUNNER_H
#define RUNNER_H

extern void bad_test_init(u32 checkins_count);
extern void bad_test_check_in();
extern void bad_test_fail(char *file,int,char *test);

#define BAD_ASSERT(cond,test_str) \
do{\
if((cond)) \
bad_test_check_in();\
else \
bad_test_fail(__FILE__,__LINE__,test_str);\
}while(0)

extern bad_task_handle_t task1h;
#define TASK1_STACK_SIZE 1024
extern u8 task1_stack[TASK1_STACK_SIZE];

extern bad_task_handle_t task2h;
#define TASK2_STACK_SIZE 1024
extern u8 task2_stack[TASK2_STACK_SIZE];

extern bad_task_handle_t task3h;
#define TASK3_STACK_SIZE 1024
extern u8 task3_stack[TASK3_STACK_SIZE];

typedef struct
{
    const bad_task_descr_t *task1_descr;
    const bad_task_descr_t *task2_descr;
    const bad_task_descr_t *task3_descr;
    void (*init_func)(void);
    void (*isr_test_func)(void);
    void (*memfault_func)(void);
    u32 num_testcases;
    const char *test_name;
} bad_test_case_t;

#endif //RUNNER_H
