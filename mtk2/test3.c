#include <stdio.h>
#include "mtk_c.h"

void task1() {
    char c[16];
    while (1) {
  	fscanf(com0in, "%s", c);
  	P(0);
        fprintf(com0out, "\r\x1b[A\x1b[K");
        fprintf(com0out, "user1: %s\n", c); 
        fprintf(com1out, "\r\x1b[A\x1b[K");
        fprintf(com1out, "user1: %s\n", c);         
        V(0);
    }	
}

void task2() {
    char c[16];
    while (1) {
        fscanf(com1in, "%s", c);
        P(0);
        fprintf(com0out, "\r\x1b[A\x1b[K");
        fprintf(com0out, "user2: %s\n", c);
        fprintf(com1out, "\r\x1b[A\x1b[K"); 
        fprintf(com1out, "user2: %s\n", c);    
        V(0);
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
