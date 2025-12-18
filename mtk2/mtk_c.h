#define DEBUG 0 // 0:normal, 1:debug

#define NULLTASKID    0 // キューの終端
#define NUMTASK       6 // 最大タスク数
#define STKSIZE    1024 // スタックサイズ
#define NUMSEMAPHORE  8 // セマフォの数

#define TASK_UNDEF    0
#define TASK_INUSE    1
#define TASK_FINISHED 2
#define TASK_READY    3
#define TASK_SLEEP    4 

typedef int TASK_ID_TYPE;
typedef int SEMAPHORE_ID_TYPE;

#define TRAP1_ID 33
extern void pv_handler(void);

typedef struct {
    int count;
    int nst;
    TASK_ID_TYPE task_list;
} SEMAPHORE_TYPE;

typedef struct {
    void (*task_addr)();
    void *stack_ptr;
    int priority;
    int status;
    TASK_ID_TYPE next;
} TCB_TYPE;
extern TCB_TYPE task_tab[NUMTASK + 1];

typedef struct {
    char ustack[STKSIZE];
    char sstack[STKSIZE];
} STACK_TYPE;
extern STACK_TYPE stacks[NUMTASK];

extern TASK_ID_TYPE curr_task;
extern TASK_ID_TYPE new_task;
extern TASK_ID_TYPE next_task;
extern TASK_ID_TYPE ready;
extern SEMAPHORE_TYPE semaphore[NUMSEMAPHORE];

/* multi task */
void init_kernel();
void set_task(void (*task_addr)());
void *init_stack(TASK_ID_TYPE id);
void begin_sch();
void addq(TCB_TYPE *q, TASK_ID_TYPE task_id);
TASK_ID_TYPE removeq(TCB_TYPE *q);
void sched();
extern void first_task();
extern void swtch();
/* timer */
extern void init_timer();
extern void skipmt();

/* smemaphore */
extern void P(int ch);
extern void V(int ch);
extern void waitP(int ch);
void sleep(int ch);
void wakeup(int ch);
void p_body(int ID);
void v_body(int ID);

/* map */
extern FILE* com0in;
extern FILE* com0out;
extern FILE* com1in;
extern FILE* com1out;
void fd_mapping();
