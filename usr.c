#include <stdio.h>
#include "mtk_c.h"

#define TERMINAL_HEIGHT 24

// ANSIシーケンス
#define CLS            "\x1b[2J"
#define SET_SCROLL     "\x1b[1;23r" 
#define SAVE_CURSOR    "\x1b[s"
#define RESTORE_CURSOR "\x1b[u"
#define GOTO_INPUT_ROW "\x1b[24;1H" 
#define ERASE_LINE     "\x1b[K"      // カーソル位置から行末まで消去

extern char inbyte(unsigned int ch);

// ログ出力関数
void write_log(FILE *target_fp, const char *user_name, const char *message) {
    fprintf(target_fp, SAVE_CURSOR);
    // 1-23行目の範囲でスクロールさせるための特殊な動き
    fprintf(target_fp, "\x1b[23;1H"); // 23行目へ移動
    fprintf(target_fp, "\n");         // ここでスクロール発生（1-23行目のみ動く）
    fprintf(target_fp, "\x1b[23;1H"); // 再度23行目へ
    fprintf(target_fp, "\x1b[K");      // その行を掃除
    fprintf(target_fp, "%s: %s", user_name, message);
    fprintf(target_fp, RESTORE_CURSOR);
}

// プレビュー表示付き my_read
int my_read_with_echo(int fd, char *buf, int nbytes, FILE *out_fp) {
    char c;
    int i, ch;
    if (fd == 0 || fd == 3) ch = 0;
    else if (fd == 4) ch = 1;
    else return -1;

    for (i = 0; i < nbytes - 1; i++) {
        c = inbyte(ch);
        
        if (c == '\r' || c == '\n') {
            buf[i] = '\0';
            return i; // 確定して戻る
        } else if (c == '\x7f' || c == '\x08') { // Backspace処理
            if (i > 0) {
                i -= 2; 
                fprintf(out_fp, "\b \b");
            } else {
                i = -1;
            }
            continue;
        } else {
            buf[i] = c;
            fputc(c, out_fp); // 入力中のプレビュー表示
        }
    }
    return i;
}

void task1() {
    char line[256];
    // 初期設定
    fprintf(com0out, CLS SET_SCROLL GOTO_INPUT_ROW "> ");

    while (1) {
        // 入力完了（Enter）まで待機
        int len = my_read_with_echo(0, line, 256, com0out);

        if (len > 0) {
            // ログを表示
            write_log(com0out, "user1", line);
            write_log(com1out, "user1", line);
        }

        // --- ここでプレビューを消去 ---
        // 24行目に移動し、行末まで消してプロンプトを再描画
        fprintf(com0out, GOTO_INPUT_ROW ERASE_LINE "> ");
    }
}

void task2() {
    char line[256];
    fprintf(com1out, CLS SET_SCROLL GOTO_INPUT_ROW "> ");

    while (1) {
        int len = my_read_with_echo(4, line, 256, com1out);

        if (len > 0) {
            write_log(com0out, "user2", line);
            write_log(com1out, "user2", line);
        }

        // プレビュー消去
        fprintf(com1out, GOTO_INPUT_ROW ERASE_LINE "> ");
    }
}

int main() {
    fd_mapping();
    init_kernel();
    
    set_task(task1);
    set_task(task2);

    begin_sch();
    return 0;
}
