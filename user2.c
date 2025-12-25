#include <stdio.h>
#include <string.h>
#include "mtk_c.h"

/* ANSIエスケープシーケンス定義 */
#define CLS          "\x1b[2J"
#define SET_SCROLL   "\x1b[1;23r"  /* 1-23行をスクロール領域に設定 */
#define SAVE_CUR     "\x1b[s"
#define REST_CUR     "\x1b[u"
#define GOTO_LOG     "\x1b[23;1H"
#define GOTO_INPUT   "\x1b[24;1H"
#define ERASE_LINE   "\x1b[K"

/* セマフォIDの定義 (NUMSEMAPHORE 8 の範囲内で使用) */
#define SCREEN_SEM   0

extern char inbyte(unsigned int ch);

/* グローバル共有バッファ */
char g_buf1[256]; int g_ready1 = 0;
char g_buf2[256]; int g_ready2 = 0;

/* --- 安全なログ出力関数 --- */
void print_safe_log(FILE *fp, const char *user, const char *msg) {
    P(SCREEN_SEM); /* 画面操作権を取得 */

    /* ログ領域を更新。SAVE/RESTOREにより、相手の入力位置を破壊しない */
    fprintf(fp, SAVE_CUR);
    fprintf(fp, GOTO_LOG "\n");
    fprintf(fp, GOTO_LOG "%s: %s", user, msg);
    fprintf(fp, REST_CUR);

    V(SCREEN_SEM); /* 画面操作権を解放 */
}

/* --- 入力タスク用ロジック --- */
void input_logic(int id, char *global_buf, int *global_ready) {
    FILE *out = (id == 0) ? com0out : com1out;
    char local_buf[256];
    int pos = 0;

    /* 初期化: 画面設定 */
    P(SCREEN_SEM);
    fprintf(out, CLS SET_SCROLL GOTO_INPUT "> ");
    V(SCREEN_SEM);

    while (1) {
        char c = inbyte(id);

        if (c == '\r' || c == '\n') {
            local_buf[pos] = '\0';
            strcpy(global_buf, local_buf);
            *global_ready = 1;

            /* 確定後、自分のプレビュー行を即座に消去 */
            P(SCREEN_SEM);
            pos = 0;
            fprintf(out, GOTO_INPUT "> " ERASE_LINE);
            V(SCREEN_SEM);
        } 
        else if (c == 0x7f || c == 0x08) { /* Backspace */
            if (pos > 0) {
                P(SCREEN_SEM);
                pos--;
                fprintf(out, "\b \b");
                V(SCREEN_SEM);
            }
        } 
        else if (pos < 250) {
            local_buf[pos++] = c;
            /* 1文字ずつのプレビュー表示もセマフォで守る */
            P(SCREEN_SEM);
            fputc(c, out);
            V(SCREEN_SEM);
        }
    }
}

void task1() { input_logic(0, g_buf1, &g_ready1); }
void task2() { input_logic(1, g_buf2, &g_ready2); }

/* --- 表示管理タスク --- */
void task_display() {
    while (1) {
        if (g_ready1) {
            print_safe_log(com0out, "user1", g_buf1);
            print_safe_log(com1out, "user1", g_buf1);
            g_ready1 = 0;
        }
        if (g_ready2) {
            print_safe_log(com0out, "user2", g_buf2);
            print_safe_log(com1out, "user2", g_buf2);
            g_ready2 = 0;
        }
        /* スケジューラを回すための短い待機 (または明示的な swtch) */
        for (volatile int i = 0; i < 2000; i++);
    }
}

int main() {
    fd_mapping();
    init_kernel();

    /* セマフォの初期化 (Mutexとして使用するため count=1) */
    semaphore[SCREEN_SEM].count = 1;
    semaphore[SCREEN_SEM].nst = 0;  
    semaphore[SCREEN_SEM].task_list = NULLTASKID;

    set_task(task1);
    set_task(task2);
    set_task(task_display);

    begin_sch();
    return 0;
}
