#include <stdio.h>
#include <string.h>
#include "mtk_c.h"

#define SCREEN_SEM 0
#define INPUT_SEM 1
#define MAX_NAME 16
#define MAX_MSG 256

extern char inkey(unsigned int);

/* ユーザー情報 */
typedef struct {
    char name[MAX_NAME];
    char color[8];
    char input_buffer[MAX_MSG];
    int input_pos;
    int is_typing;
    int status; /* statusフィールドが定義されていなかったので追加 */
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
    u->input_buffer[0] = '\0';
    u->input_pos = 0;
    u->status = 0;
    u->is_typing = 0;
}

void clear_input_line(FILE *fp) {
    fprintf(fp, "\r\x1b[2K");
}

void redraw_input(FILE *fp, User *u) {
    fprintf(fp, ">%s", u->input_buffer);
}

void send_message(User *sender, const char *msg) {
    char buf[512];
    int pos = 0;
    
    /* メッセージ送信前に両方の入力行をクリア */
    pos += sprintf(buf + pos, "\r\x1b[2K");
    
    /* メッセージ表示 */
    pos += sprintf(buf + pos, "%s%s: \x1b[0m%s\n", sender->color, sender->name, msg);
    
    /* 一度に出力 */
    fprintf(com0out, "%s", buf);
    fprintf(com1out, "%s", buf);
    
    /* 入力中だった場合は再表示 */
    if (user1.is_typing) {
        redraw_input(com0out, &user1);
    } else {
        fprintf(com0out, ">");
    }
    
    if (user2.is_typing) {
        redraw_input(com1out, &user2);
    } else {
        fprintf(com1out, ">");
    }
}

void handle_command(User *u, FILE *out_fp, const char *cmd) {
    if (strncmp(cmd, "/rename ", 8) == 0) {
        strncpy(u->name, cmd + 8, MAX_NAME - 1);
        u->name[MAX_NAME - 1] = '\0';
        
        char buf[128];
        sprintf(buf, "\r\x1b[2K\x1b[33m[System] Name changed to: %s\x1b[0m\n>", u->name);
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
            sprintf(buf, "\r\x1b[2K\x1b[33m[System] Color changed to: %s\x1b[0m\n>", color_name);
            fprintf(out_fp, "%s", buf);
        } else {
            fprintf(out_fp, "\r\x1b[2K\x1b[33m[System] Unknown color. Available: red, green, yellow, blue, magenta, cyan, white\x1b[0m\n>");
        }
    }
    else if (strcmp(cmd, "/help") == 0) {
        fprintf(out_fp, "\r\x1b[2K\x1b[33m[System] Commands:\x1b[0m\n");
        fprintf(out_fp, "  /rename <name> - Change your name\n");
        fprintf(out_fp, "  /color <color> - Change your color\n");
        fprintf(out_fp, "                   Available: red, green, yellow, blue, magenta, cyan, white\n");
        fprintf(out_fp, "  /help - Show this help\n>");
    }
    else {
        fprintf(out_fp, "\r\x1b[2K\x1b[33m[System] Unknown command. Type /help for commands\x1b[0m\n>");
    }
}


void read_message(int ch, User *u) {

    char input_char = inkey(ch);
    
    /* 入力がない場合（inkeyが-1等を返す場合）のガードが必要だが、そのまま */
    if (input_char == (char)-1) return; 

    if (input_char == '\n' || input_char == '\r') {
        if (u->input_pos > 0) {
            u->input_buffer[u->input_pos] = '\0';
            u->is_typing = 0;
            
            if (u->input_buffer[0] == '/') {
                u->status = 1;
            } else {
                u->status = 2;
            }
            
            /* ここで初期化すると送信タスクが読む前に消える可能性があるが、構造は維持 */
            u->input_pos = 0;
            /* u->input_buffer[0] = '\0'; 送信タスクが読み終わるまで消してはいけないためコメントアウト推奨 */
        }
    } else if (input_char == '\x7f' || input_char == '\x08') {
        if (u->input_pos > 0) {
            u->input_pos--;
            u->input_buffer[u->input_pos] = '\0';
            P(SCREEN_SEM);
            outbyte(ch, '\x8'); /* bs  */
            outbyte(ch, ' ');   /* spc */
            outbyte(ch, '\x8'); /* bs  */
            V(SCREEN_SEM);
        }
    } else if (input_char >= 32 && input_char <= 126) {
        /* 印字可能文字（スペース含む）のみ受け付け */
        if (u->input_pos < MAX_MSG - 1) {
            u->input_buffer[u->input_pos++] = input_char;
            u->input_buffer[u->input_pos] = '\0';
            u->is_typing = 1;
            P(SCREEN_SEM);
            outbyte(ch, input_char);
            V(SCREEN_SEM);
        }
    }
}


/* タスク1: ユーザー1の入力処理 */
void task1() {
    fprintf(com0out, "\x1b[2J\x1b[H");
    fprintf(com0out, "\x1b[33m=== CHAT SYSTEM ===\x1b[0m\n");
    fprintf(com0out, "\x1b[33mEnter your name > \x1b[0m");
    fscanf(com0in, "%s", user1.name);
    fprintf(com0out, "\x1b[33mWelcome, %s!\x1b[0m\n", user1.name);
    fprintf(com0out, "Type /help for commands\n>");
        
    while (1) {
        read_message(0, &user1); /* ポインタを渡すように修正、セミコロン追加 */
    }
}

/* タスク2: ユーザー2の入力処理 */
void task2() {
    fprintf(com1out, "\x1b[2J\x1b[H");
    fprintf(com1out, "\x1b[33m=== CHAT SYSTEM ===\x1b[0m\n");
    fprintf(com1out, "\x1b[33mEnter your name > \x1b[0m");
    fscanf(com1in, "%s", user2.name);
    fprintf(com1out, "\x1b[33mWelcome, %s!\x1b[0m\n", user2.name);
    fprintf(com1out, "Type /help for commands\n>");
    
    while (1) {
        read_message(1, &user2); /* ポインタを渡すように修正、セミコロン追加 */
    }
}

/* タスク3: user1送信タスク */
void task3() {
    while (1) {
        if(user1.status == 1) { /* ポインタではないので . に修正 */
            handle_command(&user1, com0out, user1.input_buffer);
            user1.status = 0;
        } else if(user1.status == 2) {
            send_message(&user1, user1.input_buffer);
            user1.status = 0;
        }
    }
}

/* タスク4: user2送信タスク */
void task4() {
    while (1) {
        if(user2.status == 1) { /* ポインタではないので . に修正 */
            handle_command(&user2, com1out, user2.input_buffer);
            user2.status = 0;
        } else if(user2.status == 2) {
            send_message(&user2, user2.input_buffer);
            user2.status = 0;
        }
    }
}


int main() {    
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
    set_task(task4); /* task4の登録が漏れていたので追加 */
    
    printf("[OK] All tasks set\n");
    
    begin_sch();
    
    return 0;
}
