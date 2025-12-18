/* ============================================
   対戦テトリス - マルチタスク実装
   ============================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mtk_c.h"

/* ============================================
   定数定義
   ============================================ */
#define FIELD_WIDTH     10
#define FIELD_HEIGHT    20
#define PREVIEW_SIZE    4

/* セマフォID */
#define SEM_MY_FIELD    0
#define SEM_ENEMY_FIELD 1
#define SEM_TX_BUFFER   2
#define SEM_RX_BUFFER   3

/* 通信チャネル */
#define CH_COM0         0  // 自分の入出力
#define CH_COM1         1  // 相手との通信

/* ブロックの種類 */
#define BLOCK_NONE      0
#define BLOCK_I         1
#define BLOCK_O         2
#define BLOCK_T         3
#define BLOCK_S         4
#define BLOCK_Z         5
#define BLOCK_J         6
#define BLOCK_L         7

/* ゲーム状態 */
#define GAME_READY      0
#define GAME_PLAYING    1
#define GAME_OVER       2
#define GAME_WIN        3

/* 通信パケットタイプ */
#define PKT_FIELD_UPDATE    0x01
#define PKT_SCORE_UPDATE    0x02
#define PKT_PENALTY_LINE    0x03
#define PKT_GAME_OVER       0x04
#define PKT_GAME_START      0x05

/* ============================================
   データ構造
   ============================================ */

/* テトリミノの形状定義 */
typedef struct {
    int shape[4][4];
    int size;
} Tetromino;

/* ゲームフィールド */
typedef struct {
    int field[FIELD_HEIGHT][FIELD_WIDTH];
    int score;
    int lines_cleared;
    int game_state;
} GameField;

/* 現在のブロック情報 */
typedef struct {
    int type;
    int x, y;
    int rotation;
} CurrentBlock;

/* 通信パケット */
typedef struct {
    unsigned char type;
    unsigned char data[32];
    unsigned char checksum;
} Packet;

/* ============================================
   グローバル変数
   ============================================ */

GameField my_field;
GameField enemy_field;
CurrentBlock current_block;
CurrentBlock next_block;

int game_running = 1;
int player_id = 0;  // 0 or 1

/* 送受信バッファ */
Packet tx_buffer;
Packet rx_buffer;
int tx_ready = 0;
int rx_ready = 0;

/* テトリミノ形状データ */
Tetromino tetrominos[8] = {
    // BLOCK_NONE
    {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}, 0},
    
    // BLOCK_I
    {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, 4},
    
    // BLOCK_O
    {{{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, 2},
    
    // BLOCK_T
    {{{0,0,0,0},{0,1,0,0},{1,1,1,0},{0,0,0,0}}, 3},
    
    // BLOCK_S
    {{{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}}, 3},
    
    // BLOCK_Z
    {{{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}}, 3},
    
    // BLOCK_J
    {{{0,0,0,0},{1,0,0,0},{1,1,1,0},{0,0,0,0}}, 3},
    
    // BLOCK_L
    {{{0,0,0,0},{0,0,1,0},{1,1,1,0},{0,0,0,0}}, 3}
};

/* ============================================
   関数プロトタイプ
   ============================================ */

// ゲームロジック
void init_game(void);
void spawn_block(void);
int check_collision(int x, int y, int rotation);
void fix_block(void);
int clear_lines(void);
void rotate_block(void);
void move_block(int dx, int dy);
void drop_block(void);

// 描画
void draw_screen(void);
void draw_field(GameField *field, int x_offset);
void draw_block(CurrentBlock *block, int x_offset);
void draw_score(void);

// 通信
void send_packet(unsigned char type, unsigned char *data, int len);
void process_packet(Packet *pkt);
unsigned char calc_checksum(Packet *pkt);

// タスク
void task_input(void);
void task_rx_com1(void);
void task_tx_com1(void);
void task_game_logic(void);
void task_auto_drop(void);
void task_display(void);

/* ============================================
   ゲームロジック関数
   ============================================ */

void init_game(void) {
    // フィールド初期化
    P(SEM_MY_FIELD);
    memset(&my_field, 0, sizeof(GameField));
    my_field.game_state = GAME_READY;
    V(SEM_MY_FIELD);
    
    P(SEM_ENEMY_FIELD);
    memset(&enemy_field, 0, sizeof(GameField));
    enemy_field.game_state = GAME_READY;
    V(SEM_ENEMY_FIELD);
    
    // 最初のブロック生成
    next_block.type = (rand() % 7) + 1;
    spawn_block();
}

void spawn_block(void) {
    current_block.type = next_block.type;
    current_block.x = FIELD_WIDTH / 2 - 2;
    current_block.y = 0;
    current_block.rotation = 0;
    
    next_block.type = (rand() % 7) + 1;
    
    // 出現位置で衝突 = ゲームオーバー
    if (check_collision(current_block.x, current_block.y, current_block.rotation)) {
        P(SEM_MY_FIELD);
        my_field.game_state = GAME_OVER;
        V(SEM_MY_FIELD);
        
        // ゲームオーバーを相手に通知
        unsigned char data[1] = {GAME_OVER};
        send_packet(PKT_GAME_OVER, data, 1);
    }
}

int check_collision(int x, int y, int rotation) {
    Tetromino *tet = &tetrominos[current_block.type];
    
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
            if (tet->shape[dy][dx]) {
                int fx = x + dx;
                int fy = y + dy;
                
                // 範囲外チェック
                if (fx < 0 || fx >= FIELD_WIDTH || fy >= FIELD_HEIGHT) {
                    return 1;
                }
                
                // フィールドとの衝突チェック
                if (fy >= 0) {
                    P(SEM_MY_FIELD);
                    if (my_field.field[fy][fx] != BLOCK_NONE) {
                        V(SEM_MY_FIELD);
                        return 1;
                    }
                    V(SEM_MY_FIELD);
                }
            }
        }
    }
    return 0;
}

void fix_block(void) {
    Tetromino *tet = &tetrominos[current_block.type];
    
    P(SEM_MY_FIELD);
    for (int dy = 0; dy < 4; dy++) {
        for (int dx = 0; dx < 4; dx++) {
            if (tet->shape[dy][dx]) {
                int fx = current_block.x + dx;
                int fy = current_block.y + dy;
                if (fy >= 0 && fy < FIELD_HEIGHT && fx >= 0 && fx < FIELD_WIDTH) {
                    my_field.field[fy][fx] = current_block.type;
                }
            }
        }
    }
    V(SEM_MY_FIELD);
    
    // ライン消去チェック
    int lines = clear_lines();
    if (lines > 0) {
        P(SEM_MY_FIELD);
        my_field.score += lines * lines * 100;
        my_field.lines_cleared += lines;
        V(SEM_MY_FIELD);
        
        // 2ライン以上消したら相手にペナルティ
        if (lines >= 2) {
            unsigned char data[1] = {lines - 1};
            send_packet(PKT_PENALTY_LINE, data, 1);
        }
        
        // スコア送信
        unsigned char score_data[4];
        P(SEM_MY_FIELD);
        score_data[0] = (my_field.score >> 24) & 0xFF;
        score_data[1] = (my_field.score >> 16) & 0xFF;
        score_data[2] = (my_field.score >> 8) & 0xFF;
        score_data[3] = my_field.score & 0xFF;
        V(SEM_MY_FIELD);
        send_packet(PKT_SCORE_UPDATE, score_data, 4);
    }
    
    // フィールド状態を送信
    unsigned char field_data[20];
    P(SEM_MY_FIELD);
    for (int y = 0; y < 20; y++) {
        field_data[y] = 0;
        for (int x = 0; x < 8; x++) {
            if (my_field.field[y][x] != BLOCK_NONE) {
                field_data[y] |= (1 << x);
            }
        }
    }
    V(SEM_MY_FIELD);
    send_packet(PKT_FIELD_UPDATE, field_data, 20);
    
    // 次のブロック生成
    spawn_block();
}

int clear_lines(void) {
    int lines_cleared = 0;
    
    P(SEM_MY_FIELD);
    for (int y = FIELD_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < FIELD_WIDTH; x++) {
            if (my_field.field[y][x] == BLOCK_NONE) {
                full = 0;
                break;
            }
        }
        
        if (full) {
            lines_cleared++;
            // ライン削除して上を詰める
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < FIELD_WIDTH; x++) {
                    my_field.field[yy][x] = my_field.field[yy - 1][x];
                }
            }
            // 最上行をクリア
            for (int x = 0; x < FIELD_WIDTH; x++) {
                my_field.field[0][x] = BLOCK_NONE;
            }
            y++; // 同じ行を再チェック
        }
    }
    V(SEM_MY_FIELD);
    
    return lines_cleared;
}

void rotate_block(void) {
    int new_rotation = (current_block.rotation + 1) % 4;
    if (!check_collision(current_block.x, current_block.y, new_rotation)) {
        current_block.rotation = new_rotation;
    }
}

void move_block(int dx, int dy) {
    if (!check_collision(current_block.x + dx, current_block.y + dy, current_block.rotation)) {
        current_block.x += dx;
        current_block.y += dy;
    } else if (dy > 0) {
        // 下移動で衝突 = 固定
        fix_block();
    }
}

void drop_block(void) {
    while (!check_collision(current_block.x, current_block.y + 1, current_block.rotation)) {
        current_block.y++;
    }
    fix_block();
}

/* ============================================
   通信関数
   ============================================ */

void send_packet(unsigned char type, unsigned char *data, int len) {
    P(SEM_TX_BUFFER);
    tx_buffer.type = type;
    memcpy(tx_buffer.data, data, len);
    tx_buffer.checksum = calc_checksum(&tx_buffer);
    tx_ready = 1;
    V(SEM_TX_BUFFER);
}

void process_packet(Packet *pkt) {
    // チェックサム検証
    unsigned char check = calc_checksum(pkt);
    if (check != pkt->checksum) {
        return; // パケット破損
    }
    
    switch (pkt->type) {
        case PKT_FIELD_UPDATE:
            P(SEM_ENEMY_FIELD);
            for (int y = 0; y < 20; y++) {
                for (int x = 0; x < 8; x++) {
                    if (pkt->data[y] & (1 << x)) {
                        enemy_field.field[y][x] = 1; // 簡易表示
                    } else {
                        enemy_field.field[y][x] = 0;
                    }
                }
            }
            V(SEM_ENEMY_FIELD);
            break;
            
        case PKT_SCORE_UPDATE:
            P(SEM_ENEMY_FIELD);
            enemy_field.score = (pkt->data[0] << 24) | (pkt->data[1] << 16) |
                               (pkt->data[2] << 8) | pkt->data[3];
            V(SEM_ENEMY_FIELD);
            break;
            
        case PKT_PENALTY_LINE:
            // ペナルティライン追加
            P(SEM_MY_FIELD);
            for (int i = 0; i < pkt->data[0]; i++) {
                // 一番上の行を削除
                for (int y = 0; y < FIELD_HEIGHT - 1; y++) {
                    for (int x = 0; x < FIELD_WIDTH; x++) {
                        my_field.field[y][x] = my_field.field[y + 1][x];
                    }
                }
                // 一番下にランダムな穴付きラインを追加
                int hole = rand() % FIELD_WIDTH;
                for (int x = 0; x < FIELD_WIDTH; x++) {
                    my_field.field[FIELD_HEIGHT - 1][x] = (x == hole) ? BLOCK_NONE : BLOCK_I;
                }
            }
            V(SEM_MY_FIELD);
            break;
            
        case PKT_GAME_OVER:
            P(SEM_ENEMY_FIELD);
            enemy_field.game_state = GAME_OVER;
            V(SEM_ENEMY_FIELD);
            
            P(SEM_MY_FIELD);
            if (my_field.game_state == GAME_PLAYING) {
                my_field.game_state = GAME_WIN;
            }
            V(SEM_MY_FIELD);
            break;
            
        case PKT_GAME_START:
            P(SEM_MY_FIELD);
            my_field.game_state = GAME_PLAYING;
            V(SEM_MY_FIELD);
            break;
    }
}

unsigned char calc_checksum(Packet *pkt) {
    unsigned char sum = pkt->type;
    for (int i = 0; i < 32; i++) {
        sum ^= pkt->data[i];
    }
    return sum;
}

/* ============================================
   描画関数
   ============================================ */

void draw_screen(void) {
    printf("\033[2J\033[H"); // 画面クリア
    
    printf("=== TETRIS BATTLE ===\n");
    printf("  YOUR FIELD          ENEMY FIELD\n");
    
    // 2つのフィールドを並べて表示
    P(SEM_MY_FIELD);
    P(SEM_ENEMY_FIELD);
    
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        // 自分のフィールド
        printf("|");
        for (int x = 0; x < FIELD_WIDTH; x++) {
            if (my_field.field[y][x] != BLOCK_NONE) {
                printf("[]");
            } else {
                printf("  ");
            }
        }
        printf("|  ");
        
        // 相手のフィールド
        printf("|");
        for (int x = 0; x < FIELD_WIDTH; x++) {
            if (enemy_field.field[y][x] != BLOCK_NONE) {
                printf("[]");
            } else {
                printf("  ");
            }
        }
        printf("|\n");
    }
    
    printf("+");
    for (int i = 0; i < FIELD_WIDTH * 2; i++) printf("-");
    printf("+  +");
    for (int i = 0; i < FIELD_WIDTH * 2; i++) printf("-");
    printf("+\n");
    
    printf("Score: %d          Score: %d\n", my_field.score, enemy_field.score);
    printf("Lines: %d          Lines: %d\n", my_field.lines_cleared, enemy_field.lines_cleared);
    
    if (my_field.game_state == GAME_OVER) {
        printf("\n*** GAME OVER ***\n");
    } else if (my_field.game_state == GAME_WIN) {
        printf("\n*** YOU WIN! ***\n");
    }
    
    V(SEM_ENEMY_FIELD);
    V(SEM_MY_FIELD);
    
    printf("\nControls: A=Left, D=Right, S=Down, W=Rotate, SPACE=Drop, Q=Quit\n");
}

/* ============================================
   タスク定義
   ============================================ */

void task_input(void) {
    while (game_running) {
        char buf[256];
        int size = GETSTRING(CH_COM0, buf, 1);
        
        if (size > 0) {
            P(SEM_MY_FIELD);
            if (my_field.game_state == GAME_PLAYING) {
                V(SEM_MY_FIELD);
                
                switch (buf[0]) {
                    case 'a': case 'A':
                        move_block(-1, 0);
                        break;
                    case 'd': case 'D':
                        move_block(1, 0);
                        break;
                    case 's': case 'S':
                        move_block(0, 1);
                        break;
                    case 'w': case 'W':
                        rotate_block();
                        break;
                    case ' ':
                        drop_block();
                        break;
                    case 'q': case 'Q':
                        game_running = 0;
                        break;
                }
            } else {
                V(SEM_MY_FIELD);
            }
        }
        skipmt(); // 他のタスクに譲る
    }
}

void task_rx_com1(void) {
    while (game_running) {
        unsigned char buf[64];
        int size = GETSTRING(CH_COM1, buf, sizeof(Packet));
        
        if (size == sizeof(Packet)) {
            P(SEM_RX_BUFFER);
            memcpy(&rx_buffer, buf, sizeof(Packet));
            rx_ready = 1;
            V(SEM_RX_BUFFER);
        }
        skipmt();
    }
}

void task_tx_com1(void) {
    while (game_running) {
        P(SEM_TX_BUFFER);
        if (tx_ready) {
            unsigned char *buf = (unsigned char *)&tx_buffer;
            PUTSTRING(CH_COM1, buf, sizeof(Packet));
            tx_ready = 0;
        }
        V(SEM_TX_BUFFER);
        skipmt();
    }
}

void task_game_logic(void) {
    while (game_running) {
        P(SEM_RX_BUFFER);
        if (rx_ready) {
            process_packet(&rx_buffer);
            rx_ready = 0;
        }
        V(SEM_RX_BUFFER);
        skipmt();
    }
}

void task_auto_drop(void) {
    // 1秒ごとに自動落下
    while (game_running) {
        P(SEM_MY_FIELD);
        if (my_field.game_state == GAME_PLAYING) {
            V(SEM_MY_FIELD);
            move_block(0, 1);
        } else {
            V(SEM_MY_FIELD);
        }
        
        // 1秒待機（簡易実装）
        for (volatile int i = 0; i < 1000000; i++);
    }
}

void task_display(void) {
    while (game_running) {
        draw_screen();
        
        // 画面更新間隔
        for (volatile int i = 0; i < 500000; i++);
    }
}

/* ============================================
   メイン関数
   ============================================ */

void start(void) {
    printf("Tetris Battle - Starting...\n");
    
    // プレイヤーID決定（簡易）
    printf("Player ID (0 or 1): ");
    char buf[256];
    GETSTRING(CH_COM0, buf, 256);
    player_id = buf[0] - '0';
    
    // ゲーム初期化
    init_game();
    
    // タスク登録
    set_task(task_input);
    set_task(task_rx_com1);
    set_task(task_tx_com1);
    set_task(task_game_logic);
    set_task(task_auto_drop);
    set_task(task_display);
    
    // ゲーム開始通知
    if (player_id == 0) {
        unsigned char data[1] = {GAME_PLAYING};
        send_packet(PKT_GAME_START, data, 1);
    }
    
    P(SEM_MY_FIELD);
    my_field.game_state = GAME_PLAYING;
    V(SEM_MY_FIELD);
    
    // スケジューラ開始
    begin_sch();
}
