#include <stdio.h>
#include <stdint.h>
#include "mtk_c.h"
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>

TASK_ID_TYPE curr_task;
TASK_ID_TYPE new_task;
TASK_ID_TYPE next_task;
TASK_ID_TYPE ready;
TCB_TYPE task_tab[NUMTASK + 1];
STACK_TYPE stacks[NUMTASK];
SEMAPHORE_TYPE semaphore[NUMSEMAPHORE];

void init_kernel() {
    // TCB配列を初期化する
    for (int i = 0; i < NUMTASK; i++) {
        TCB_TYPE *tcb = &task_tab[i + 1];
        tcb->task_addr = NULL;
        tcb->stack_ptr = NULL;
        tcb->priority = 0;
        tcb->status = TASK_UNDEF;
        tcb->next = NULLTASKID;
    }
    // readyキューを初期化する
    ready = NULLTASKID;
    // pv_handlerをTRAP #1の割り込みベクタに登録
    *(int *)(TRAP1_ID * 4) = (int)(uintptr_t)pv_handler;
    // セマフォの値を初期化する
    for (int i = 0; i < NUMSEMAPHORE; i++) {
        SEMAPHORE_TYPE *sema = &semaphore[i];
        sema->count = 1; // 1: 利用可能, <1: 待ちタスクがある
        sema->nst = 0;   // たぶんまだ使わない
        sema->task_list = NULLTASKID;
    }
}

void set_task(void (*task_addr)()) {
    // タスクIDの決定
    TASK_ID_TYPE task_id = NULLTASKID;
    for (int i = 0; i < NUMTASK; i++) {
        TCB_TYPE *tcb = &task_tab[i + 1];
        if (tcb->status == TASK_UNDEF || tcb->status == TASK_FINISHED) {
            task_id = i + 1;
            break;
        }
    }
    if (task_id == NULLTASKID) {printf("full"); return;} // 空きがない
    new_task = task_id; // 空いていたTCBのIDをnew_taskに代入

    TCB_TYPE *tcb = &task_tab[new_task];
    tcb->task_addr = task_addr;    // task_addrを登録
    tcb->status = TASK_INUSE;      // statusを登録
    tcb->stack_ptr = init_stack(new_task); // stack_ptrを登録

    if (ready == NULLTASKID) {
        ready = new_task; // readyキューが空ならnew_taskを追加
        task_tab[ready].next = NULLTASKID;
    }
    else addq(&task_tab[ready], new_task);
    printf("[OK] set_task\n");
}

void begin_sch() {
    curr_task = removeq(&task_tab[ready]); // 最初のタスクの決定
    if (DEBUG) printf("[DEBUG] curr_task = %d\n", curr_task);
    init_timer(); // タイマの設定
    printf("[OK] init_timer\n");
    first_task(); // 最初のタスクへ遷移
}

void *init_stack(TASK_ID_TYPE id) {
    char *ustack_top = &stacks[id - 1].ustack[STKSIZE];
    char *sstack = stacks[id - 1].sstack;
    int *ssp = (int *)(sstack + STKSIZE);
    *(--ssp) = (int)(uintptr_t)task_tab[id].task_addr; // initial PC
    short *ssp_s = (short*)ssp;
    *(--ssp_s) = (short)0x0000; // initial SR
    ssp = (int*)ssp_s;
    ssp -= 15; // 15x4 bytes for registers
    *(--ssp) = (int)(uintptr_t)ustack_top;
    return ssp;
}

void addq(TCB_TYPE* q_ptr, TASK_ID_TYPE task_id) {
    if (DEBUG) printf("[DEBUG] addq: added task_id = %d\n", task_id);
    // 引数にキューへのポインタとタスクの ID を取り，その TCB をキューの最後尾に登録する．
    TCB_TYPE *cur = q_ptr;
    while (cur->next != NULLTASKID) cur = &task_tab[cur->next];
    // ここに到達した時点でcur->nextはNULLTASKID
    cur->next = task_id; // 最後尾に追加
    task_tab[task_id].next = NULLTASKID; // 新しい最後尾
}

TASK_ID_TYPE removeq(TCB_TYPE* q_ptr) {
    TASK_ID_TYPE task_id;
    // semaphore キューの場合
    for (int i = 0; i < NUMSEMAPHORE; i++) {
        if (q_ptr == &task_tab[semaphore[i].task_list]) {
            task_id = semaphore[i].task_list; // task_id = キューの先頭タスクID
            semaphore[i].task_list = (*q_ptr).next; // 先頭タスクをキューから取り除く
            // (*q_ptr).next = NULLTASKID;
            if (DEBUG) printf("[DEBUG] removeq returns %d (semaphore queue)\n", task_id);
            return task_id;
        }
    }

    // ready キューの場合
    if (q_ptr == &task_tab[ready]) {
        task_id = ready; // task_id = キューの先頭タスクID
        ready = (*q_ptr).next; // 先頭タスクをキューから取り除く
        // (*q_ptr).next = NULLTASKID;
        if (DEBUG) printf("[DEBUG] removeq returns %d (ready queue)\n", task_id);
        return task_id;
    }
    return NULLTASKID;
}

void sched() {
    if (DEBUG) printf("[DEBUG] sched()\n");
    next_task = removeq(&task_tab[ready]); // next_task = readyキューの先頭タスクID
    if (DEBUG) printf("[DEBUG] sched: next_task = %d\n", next_task);
    while (next_task == NULLTASKID); // next_task = NULLTASKID なら無限ループ 
}

void p_body(int ID) {
    if (DEBUG) printf("[DEBUG] p_body(%d)\n", ID);
    // セマフォIDがスタックに積まれている
    // 1.セマフォの値を減らす
    SEMAPHORE_TYPE *sema = &semaphore[ID];
    sema->count -= 1;
    // 2.セマフォが獲得できなけれれば sleep(セマフォの ID)
    if (sema->count < 0) sleep(ID);
}

void waitp_body(SEMAPHORE_ID_TYPE sem_id) {
    SEMAPHORE_TYPE *sp;
    sp = &semaphore[sem_id];
    if (sp->count != -(sp->nst - 1)) {
        p_body(sem_id);
    } else {
	for (int k = 0; k < sp->nst - 1; k++) {
	    v_body(sem_id);
	addq(&task_tab[ready], curr_task);
	sched();
	swtch();
	}	
    }
}

void v_body(int ID) {
    if (DEBUG) printf("[DEBUG] v_body(%d)\n", ID);
    // セマフォIDがスタックに積まれている
    // 1.セマフォの値を増やす
    SEMAPHORE_TYPE *sema = &semaphore[ID];
    sema->count += 1;
    // 2.セマフォが空けば，wakeup(セマフォの ID) 
    if (sema->count <= 0) wakeup(ID);
}

void sleep(int ch) {
    if (DEBUG) printf("[DEBUG] sleep(%d)\n", ch);
    SEMAPHORE_TYPE *sema = &semaphore[ch]; /*セマフォのポインタの取得p38*/
    if (sema->task_list == NULLTASKID) {
        sema->task_list = curr_task;
        task_tab[curr_task].next = NULLTASKID;
    }
    else addq(&task_tab[sema->task_list], curr_task); /*現在実行中のタスクcurrent_taskを、セマフォの待ち行列(task_list)の末尾に追加する。*/
    task_tab[curr_task].status = TASK_SLEEP; /*タスクの状態を管理(TCBのstatusを管理する)*/
    sched();
    swtch();    
}

void wakeup(int ch){
    if (DEBUG) printf("[DEBUG] wakeup(%d)\n", ch);
    SEMAPHORE_TYPE *sema = &semaphore[ch];
    TASK_ID_TYPE woken_task_id = removeq(&task_tab[sema->task_list]);
    
    if (woken_task_id != NULLTASKID) {
        if (ready == NULLTASKID) {
            ready = woken_task_id; 
            task_tab[ready].next = NULLTASKID;
        }
        else addq(&task_tab[ready], woken_task_id);
        task_tab[woken_task_id].status = TASK_READY;
    }
}

FILE* com0in;
FILE* com0out;
FILE* com1in;
FILE* com1out;

void fd_mapping() {
    com0in  = fdopen(3, "r");
    com0out = fdopen(3, "w");
    com1in  = fdopen(4, "r");
    com1out = fdopen(4, "w");
}

