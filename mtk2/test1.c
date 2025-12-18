#include <stdio.h>

int main() {
    char buf[16];
    while (1) {
        printf("please input > ");
        scanf("%s", buf);
        printf("%s\n",buf);
    }
}

void exit(int value) {
    *(char *)0x00d00039 = 'H';
    for (;;);
}
