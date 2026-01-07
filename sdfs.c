#include <stdio.h>
#include <string.h>
#include "mtk_c.h"

/* セマフォID定義 */
#define SCREEN_SEM 0
#define INPUT_SEM  1

/* 定数定義 */
#define MAX_NAME 16
#define MAX_MSG  256

/* 外部アセンブリ関数・ライブラリ関数の宣言 */
extern char inkey(unsigned int port);
extern void outbyte(unsigned int port, char c);

/* ユーザー状態管理構造体 */
typedef struct {
    char name[MAX_NAME];
    char color[8];
    char input_buffer[MAX_MSG];  /* 現在入力中の文字列 */
    char send_buffer[MAX_MSG];   /* 送信・処理待ちの文字列 */
    int input_pos;               /* 入力カーソル位置 */
    int is_typing;               /* 入力中フラグ */
    int status;                  /* 0:待機, 1:コマンド実行待ち, 2:メッセージ送信待ち */
} User;

/* グローバル変数: 2人のユーザー情報 */
User user1, user2;

/* ANSIエスケープシーケンスによる色定義 */
const char *colors[] = {
    "\x1b[31m", /* 0: red */
    "\x1b[32m", /* 1: green */
    "\x1b[33m", /* 2: yellow */
    "\x1b[34m", /* 3: blue */
    "\x1b[35m", /* 4: magenta */
    "\x1b[36m", /* 5: cyan */
    "\x1b[37m"  /* 6: white */
};

/* ユーザー情報の初期化 */
void init_user(User *u, const char *default_name, const char *default_color) {
    strncpy(u->name, default_name, MAX_NAME - 1);
    strcpy(u->color, default_color);
    u->input_buffer[0] = '\0';
    u->send_buffer[0] = '\0';
    u->input_pos = 0;
    u->status = 0;
    u->is_typing = 0;
}

/* 入力プロンプトの再描画 */
void redraw_input(FILE *fp, User *u) {
    fprintf(fp, ">%s", u->input_buffer);
}

/* メッセージの全体送信（ブロードキャスト） */
void send_message(User *sender, const char *msg) {
    char buf[512];
    
    /* 1. 現在の入力行をクリア (\r: 行頭復帰, \x1b[2K: 行消去) */
    sprintf(buf, "\r\x1b[2K%s%s: \x1b[0m%s\n", sender->color, sender->name, msg);
    
    /* 2. 両方のシリアルポートへ出力 */
    fprintf(com0out, "%s", buf);
    fprintf(com1out, "%s", buf);
    
    /* 3. 各自の入力中だった文字列を再表示してプロンプトを復元 */
    if (user1.is_typing) redraw_input(com0out, &user1);
    else fprintf(com0out, ">");
    
    if (user2.is_typing) redraw_input(com1out, &user2);
    else fprintf(com1out, ">");
}

/* システムコマンドの処理 */
void handle_command(User *u, FILE *out_fp, const char *cmd) {
    fprintf(out_fp, "\r\x1b[2K"); /* プロンプト行を一時消去 */
    
    if (strncmp(cmd, "/rename ", 8) == 0) {
        strncpy(u->name, cmd + 8, MAX_NAME - 1);
        u->name[MAX_NAME - 1] = '\0';
        fprintf(out_fp, "\x1b[33m[System] Name changed to: %s\x1b[0m\n", u->name);
    }
    else if (strncmp(cmd, "/color ", 7) == 0) {
        const char *c_name = cmd + 7;
        int idx = -1;
        if      (strcmp(c_name, "red") == 0)     idx = 0;
        else if (strcmp(c_name, "green") == 0)   idx = 1;
        else if (strcmp(c_name, "yellow") == 0)  idx = 2;
        else if (strcmp(c_name, "blue") == 0)    idx = 3;
        else if (strcmp(c_name, "magenta") == 0) idx = 4;
        else if (strcmp(c_name, "cyan") == 0)    idx = 5;
        else if (strcmp(c_name, "white") == 0)   idx = 6;

        if (idx != -1) {
            strcpy(u->color, colors[idx]);
            fprintf(out_fp, "\x1b[33m[System] Color updated to %s.\x1b[0m\n", c_name);
        } else {
            fprintf(out_fp, "\x1b[31m[System] Error: Color '%s' not found.\x1b[0m\n", c_name);
        }
    }
    else if (strcmp(cmd, "/help") == 0) {
        fprintf(out_fp, "\x1b[33m[Help] /rename <name>, /color <color>, /help\x1b[0m\n");
    }
    else {
        fprintf(out_fp, "\x1b[31m[System] Unknown command. Type /help\x1b[0m\n");
    }
    fprintf(out_fp, ">");
}

/* 入力解析ルーチン */
void read_input_step(int port, User *u) {
    int c = inkey(port);
    if (c == -1) return; /* 入力がない場合は何もしない（タスク切り替えへ） */

    char ch = (char)c;

    /* エンターキー: 送信処理へ */
    if (ch == '\n' || ch == '\r') {
        if (u->input_pos > 0) {
            u->input_buffer[u->input_pos] = '\0';
            strcpy(u->send_buffer, u->input_buffer); /* 送信タスクへ渡すバッファにコピー */
            u->is_typing = 0;
            u->status = (u->input_buffer[0] == '/') ? 1 : 2; /* 1:コマンド, 2:メッセージ */
            u->input_pos = 0;
            u->input_buffer[0] = '\0';
        }
    } 
    /* バックスペース処理 */
    else if (ch == '\x7f' || ch == '\x08') {
        if (u->input_pos > 0) {
            u->input_pos--;
            u->input_buffer[u->input_pos] = '\0';
            P(SCREEN_SEM);
            outbyte(port, '\x08'); outbyte(port, ' '); outbyte(port, '\x08');
            V(SCREEN_SEM);
        }
    } 
    /* 通常文字入力 */
    else if (ch >= 32 && ch <= 126) {
        if (u->input_pos < MAX_MSG - 1) {
            u->input_buffer[u->input_pos++] = ch;
            u->input_buffer[u->input_pos] = '\0';
            u->is_typing = 1;
            P(SCREEN_SEM);
            outbyte(port, ch);
            V(SCREEN_SEM);
        }
    }
}

/* --- タスク定義 --- */

/* タスク1: User1(COM0) 入力監視 */
void task1() {
    fprintf(com0out, "\x1b[2J\x1b[H\x1b[32m=== CHAT SYSTEM (User1: COM0) ===\x1b[0m\n>");
    while (1) {
        read_input_step(0, &user1);
    }
}

/* タスク2: User2(COM1) 入力監視 */
void task2() {
    fprintf(com1out, "\x1b[2J\x1b[H\x1b[36m=== CHAT SYSTEM (User2: COM1) ===\x1b[0m\n>");
    while (1) {
        read_input_step(1, &user2);
    }
}

/* タスク3: User1用 送信・実行タスク */
void task3() {
    while (1) {
        if (user1.status != 0) {
            P(SCREEN_SEM);
            if (user1.status == 1) handle_command(&user1, com0out, user1.send_buffer);
            else if (user1.status == 2) send_message(&user1, user1.send_buffer);
            V(SCREEN_SEM);
            user1.status = 0; /* 処理完了 */
        }
    }
}

/* タスク4: User2用 送信・実行タスク */
void task4() {
    while (1) {
        if (user2.status != 0) {
            P(SCREEN_SEM);
            if (user2.status == 1) handle_command(&user2, com1out, user2.send_buffer);
            else if (user2.status == 2) send_message(&user2, user2.send_buffer);
            V(SCREEN_SEM);
            user2.status = 0; /* 処理完了 */
        }
    }
}

/* メイン関数 */
int main() {    
    /* 1. ハードウェア・カーネルの初期化 */
    fd_mapping();
    init_kernel();
    
    /* 2. セマフォ初期化 (バイナリセマフォとして使用) */
    semaphore[SCREEN_SEM].count = 1;
    semaphore[INPUT_SEM].count = 1;
    
    /* 3. ユーザー情報の初期化 */
    init_user(&user1, "User1", colors[1]); /* Green */
    init_user(&user2, "User2", colors[5]); /* Cyan */
    
    /* 4. タスクの登録 */
    set_task(task1);
    set_task(task2);
    set_task(task3);
    set_task(task4);
    
    /* 5. スケジューリング開始 */
    begin_sch();
    
    return 0;
}
