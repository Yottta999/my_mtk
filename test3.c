#include <stdio.h>
#include "mtk_c.h"

void task1() {
    char c;
    while (1) {
  	fscanf(com0in, "%c", &c);
        fprintf(com0out, "%c", c);   
    }	
}

void task2() {
    char c;
    while (1) {
        fscanf(com0in, "%c", &c);
        fprintf(com0out, "%c", c);
    }   
}

int main() {
    printf("BOOTING\n");
    fd_mapping();
    printf("[OK] fd_mapping\n");
    init_kernel();
    printf("[OK] init_kernel\n");

    set_task(task1);
    set_task(task2);
    
    begin_sch();
}
