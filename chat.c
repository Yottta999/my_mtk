#include <stdio.h>
#include <string.h>
#include "mtk_c.h"

#define SCREEN_SEM 0
#define INPUT_SEM 1
#define MAX_NAME 16
#define MAX_MSG 256

extern char inkey(unsigned int ch);
/* ユーザー情報 */
typedef struct {
    char name[MAX_NAME];
    char color[8];
    char input_buffer[MAX_MSG];
    int input_pos;
    int is_typing;
} User;

User user1, user2;
int system_ready = 0;

/* 色コード定義 */
const char *colors[] = {
    "\x1b[31m", /* red */
    "\x1b[32m", /* green */
    "\x1b[33m", /* yellow */
    "\x1b[34m", /* blue */
    "\x1b[35m", /* magenta */
    "\x1b[36m", /* cyan */
    "\x1b[37m"  /* white */
};

void init_user(User *u, const char *default_name, const char *default_color) {
    strcpy(u->name, default_name);
    strcpy(u->color, default_color);
    u->input_buffer[0] = '\0';  /* バッファを空文字列に初期化 */
    u->input_pos = 0;            /* 入力位置を0に初期化 */
    u->is_typing = 0;            /* 入力中フラグをオフに初期化 */
}

void clear_input_line(FILE *fp) {
    fprintf(fp, "\r\x1b[2K");
}

void redraw_input(FILE *fp, User *u) {
    fprintf(fp, ">%s", u->input_buffer);
}

int total_messages_sent = 0;

void send_message(User *sender, const char *msg) {
    char buf[512];
    int pos = 0;

    /* 両方の画面の入力行を消去してからメッセージ表示 */
    pos += sprintf(buf + pos, "\r\x1b[2K");
    pos += sprintf(buf + pos, "%s%s: \x1b[0m%s\n", sender->color, sender->name, msg);
    
    /* 一度に出力 */
    fprintf(com0out, "%s", buf);
    fprintf(com1out, "%s", buf);
    fprintf(com0out, "\x1b[K");
    fprintf(com1out, "\x1b[K");

    // User1の画面
    if (user1.is_typing) {
        fprintf(com0out, ">%s", user1.input_buffer);
    } else {
        fprintf(com0out, ">");
    }

    // User2の画面
    if (user2.is_typing) {
        fprintf(com1out, ">%s", user2.input_buffer);
    } else {
        fprintf(com1out, ">");
    }
}

void handle_command(User *u, FILE *out_fp, const char *cmd) {
    if (strncmp(cmd, "/rename ", 8) == 0) {
        strncpy(u->name, cmd + 8, MAX_NAME - 1);
        u->name[MAX_NAME - 1] = '\0';
        
        char buf[128];
        sprintf(buf, "\r\x1b[2K\x1b[33m[System] Name changed to: %s\x1b[0m\n\x1b[K>", u->name);
        fprintf(out_fp, "%s", buf);
    }
    else if (strncmp(cmd, "/color ", 7) == 0) {
        const char *color_name = cmd + 7;
        int found = 0;
        
        if (strcmp(color_name, "red") == 0) {
            strcpy(u->color, colors[0]);
            found = 1;
        } else if (strcmp(color_name, "green") == 0) {
            strcpy(u->color, colors[1]);
            found = 1;
        } else if (strcmp(color_name, "yellow") == 0) {
            strcpy(u->color, colors[2]);
            found = 1;
        } else if (strcmp(color_name, "blue") == 0) {
            strcpy(u->color, colors[3]);
            found = 1;
        } else if (strcmp(color_name, "magenta") == 0) {
            strcpy(u->color, colors[4]);
            found = 1;
        } else if (strcmp(color_name, "cyan") == 0) {
            strcpy(u->color, colors[5]);
            found = 1;
        } else if (strcmp(color_name, "white") == 0) {
            strcpy(u->color, colors[6]);
            found = 1;
        }
        
        if (found) {
            char buf[128];
            sprintf(buf, "\r\x1b[2K\x1b[33m[System] Color changed to: %s\x1b[0m\x1b[K>", color_name);
            fprintf(out_fp, "%s", buf);
        } else {
            fprintf(out_fp, "\r\x1b[2K\x1b[33m[System] Unknown color. Available: red, green, yellow, blue, magenta, cyan, white\x1b[0m\n>");
        }
    }
    else if (strcmp(cmd, "/help") == 0) {
        fprintf(out_fp, "\r\x1b[2K\x1b[33m[System] Commands:\x1b[0m\n");
        fprintf(out_fp, "  /rename <name> - Change your name\n");
        fprintf(out_fp, "  /color <color> - Change your color\n                   {green, yellow, blue, magenta, cyan, white}\n");
        fprintf(out_fp, "  /help - Show this help\n>");
    }
    else {
        fprintf(out_fp, "\r\x1b[2K\x1b[33m[System] Unknown command. Type /help for commands\x1b[0m\n>");
    }
}

/* タスク1: ユーザー1の入力処理 */
void task1() {
    char input_char;
    
    fprintf(com0out, "\x1b[2J\x1b[H");
    fprintf(com0out, "\x1b[33m=== CHAT SYSTEM ===\x1b[0m\n");
    fprintf(com0out, "\x1b[33mEnter your name > \x1b[0m");
    
    /* 名前入力 */
    int name_idx = 0;
    P(3);
    while (1) {
        input_char = fgetc(com0in);
        if (input_char == '\n' || input_char == '\r') {
            user1.name[name_idx] = '\0';
            break;
        } else if (input_char == '\x7f' || input_char == '\x08') {
            if (name_idx > 0) {
                name_idx--;
                fprintf(com0out, "\x08 \x08");
            }
        } else if (input_char >= 32 && input_char <= 126 && name_idx < MAX_NAME - 1) {
            user1.name[name_idx++] = input_char;
        }
    }
    
    fprintf(com0out, "\x1b[33mWelcome, %s!\x1b[0m\n>", user1.name);
    V(3);
    P(4);
    while (1) {
        input_char = fgetc(com0in);
        
        P(INPUT_SEM);
        
        if (input_char == '\n' || input_char == '\r') {
            if (user1.input_pos > 0) {
                user1.input_buffer[user1.input_pos] = '\0';
                user1.is_typing = 0;
                
                P(SCREEN_SEM);
                fprintf(com0out, "\r\x1b[A\x1b[2K");
                if (user1.input_buffer[0] == '/') {
                    /* コマンド実行（自分の画面のみ消去） */
                    handle_command(&user1, com0out, user1.input_buffer);
                } else {
                    /* メッセージ送信（send_message内で両画面消去） */
                    send_message(&user1, user1.input_buffer);
                }
                
                user1.input_pos = 0;
                user1.input_buffer[0] = '\0';
                V(SCREEN_SEM);
            } else {
                /* 空行の場合はプロンプトだけ再表示 */
                P(SCREEN_SEM);
                fprintf(com0out, "\r\x1b[2K>");
                V(SCREEN_SEM);
            }
        } else if (input_char == '\x7f' || input_char == '\x08') {
            if (user1.input_pos > 0) {
                user1.input_pos--;
                user1.input_buffer[user1.input_pos] = '\0';
                P(SCREEN_SEM);
                fprintf(com0out, "\x08 \x08");
                V(SCREEN_SEM);
            }
        } else if (input_char >= 32 && input_char <= 126) {
            /* 印字可能文字（スペース含む）のみ受け付け */
            if (user1.input_pos < MAX_MSG - 1) {
                user1.input_buffer[user1.input_pos++] = input_char;
                user1.input_buffer[user1.input_pos] = '\0';
                user1.is_typing = 1;
                P(SCREEN_SEM);
                fprintf(com0out, "%c", input_char);
                V(SCREEN_SEM);
            }
        }
        
        V(INPUT_SEM);
    }
}

/* タスク2: ユーザー2の入力処理 */
void task2() {
    char input_char;
    
    fprintf(com1out, "\x1b[2J\x1b[H");
    fprintf(com1out, "\x1b[33m=== CHAT SYSTEM ===\x1b[0m\n");
    fprintf(com1out, "\x1b[33mEnter your name > \x1b[0m");
    
    /* 名前入力 */
    int name_idx = 0;
    P(4);
    while (1) {
        input_char = fgetc(com1in);
        if (input_char == '\n' || input_char == '\r') {
            user2.name[name_idx] = '\0';
            break;
        } else if (input_char == '\x7f' || input_char == '\x08') {
            if (name_idx > 0) {
                name_idx--;
                fprintf(com1out, "\x08 \x08");
            }
        } else if (input_char >= 32 && input_char <= 126 && name_idx < MAX_NAME - 1) {
            user2.name[name_idx++] = input_char;
        }
    }
    
    fprintf(com1out, "\x1b[33mWelcome, %s!\x1b[0m\n>", user2.name);
    V(4);
    P(3);
    while (1) {
        input_char = fgetc(com1in);
        
        P(INPUT_SEM);
        
        if (input_char == '\n' || input_char == '\r') {
            if (user2.input_pos > 0) {
                user2.input_buffer[user2.input_pos] = '\0';
                user2.is_typing = 0;
                
                P(SCREEN_SEM);
                fprintf(com1out, "\r\x1b[A\x1b[2K");
                if (user2.input_buffer[0] == '/') {
                    /* コマンド実行（自分の画面のみ消去） */
                    handle_command(&user2, com1out, user2.input_buffer);
                } else {
                    /* メッセージ送信（send_message内で両画面消去） */
                    send_message(&user2, user2.input_buffer);
                }
                
                user2.input_pos = 0;
                user2.input_buffer[0] = '\0';
                V(SCREEN_SEM);
            } else {
                /* 空行の場合はプロンプトだけ再表示 */
                P(SCREEN_SEM);
                fprintf(com1out, "\r\x1b[2K>");
                V(SCREEN_SEM);
            }
        } else if (input_char == '\x7f' || input_char == '\x08') {
            if (user2.input_pos > 0) {
                user2.input_pos--;
                user2.input_buffer[user2.input_pos] = '\0';
                P(SCREEN_SEM);
                fprintf(com1out, "\x08 \x08");
                V(SCREEN_SEM);
            }
        } else if (input_char >= 32 && input_char <= 126) {
            /* 印字可能文字（スペース含む）のみ受け付け */
            if (user2.input_pos < MAX_MSG - 1) {
                user2.input_buffer[user2.input_pos++] = input_char;
                user2.input_buffer[user2.input_pos] = '\0';
                user2.is_typing = 1;
                P(SCREEN_SEM);
                fprintf(com1out, "%c", input_char);
                V(SCREEN_SEM);
            }
        }
        
        V(INPUT_SEM);
    }
}

/* タスク3: 入力状態監視・再描画タスク */
void task3() {
    int prev_typing1 = 0;
    int prev_typing2 = 0;
    
    while (1) {
        for (int i = 0; i < 500000; i++);
        
        P(INPUT_SEM);
        
        /* 入力状態が変化したら再描画 */
        if (prev_typing1 != user1.is_typing || prev_typing2 != user2.is_typing) {
            prev_typing1 = user1.is_typing;
            prev_typing2 = user2.is_typing;
        }
        
        V(INPUT_SEM);
    }
}



int main() {
    printf("CHAT SYSTEM INITIALIZING\n");
    
    fd_mapping();
    printf("[OK] fd_mapping\n");
    
    init_kernel();
    printf("[OK] init_kernel\n");
    
    /* セマフォの初期化 */
    semaphore[SCREEN_SEM].count = 1;
    semaphore[SCREEN_SEM].nst = 0;
    semaphore[INPUT_SEM].count = 1;
    semaphore[INPUT_SEM].nst = 0;
    
    /* ユーザー初期化 */
    init_user(&user1, "User1", colors[1]); /* green */
    init_user(&user2, "User2", colors[0]); /* red */
    
    printf("[OK] Users initialized\n");
    
    set_task(task1);
    set_task(task2);
    set_task(task3);
    
    printf("[OK] All tasks set\n");
    
    begin_sch();
    
    return 0;
}
