#include <stdio.h>
#include "mtk_c.h"

void dump() {
    printf("[DEBUG] dump current state\n");
    printf("curr_task = %d, next_task = %d\n", curr_task, next_task);
    printf("[DEBUG] dump ready queue\n");
    printf("ready queue: ");
    for (TASK_ID_TYPE id = ready; id != NULLTASKID; id = task_tab[id].next) {
        TCB_TYPE *cur = &task_tab[id];
        printf("%d(%d) -> ", id, (int) cur->task_addr);
    }
    printf("NULLTASKID\n");
    printf("[DEBUG] dump semaphore queue\n");
    for (int id = 0; id < NUMSEMAPHORE; id++) {
        SEMAPHORE_TYPE *sema = &semaphore[id];
        if (sema->task_list == NULLTASKID) continue;
        TASK_ID_TYPE task_id = sema->task_list;
        printf("semaphore #%d: ", id);
        do {
            printf("%d -> ", task_id);
            task_id = task_tab[task_id].next;
        } while (task_id != NULLTASKID);
        printf("NULLTASKID\n");
    }
}

void task1() {
    while (1) {
	printf("task1 \n");
        waitP(1);
    }
}

void task2() {
    while (1) {
        printf("task2\n");
        waitP(0);
    }
}

void task3() {
    while (1) {
        printf("task3\n");
        waitP(0);
    }
}

int main() {
    printf("BOOTING\n");
    init_kernel();
    printf("[OK] init_kernel\n");
    
    semaphore[1].count =0;
    semaphore[1].nst = 3;
    set_task(task1);
    set_task(task2);
    set_task(task3);

    if (DEBUG) printf("[DEBUG] sizeof TCB_TYPE = %ld\n", sizeof(TCB_TYPE));
    if (DEBUG) printf("[DEBUG] ready = %d\n", ready);
    dump();

    begin_sch();
}
