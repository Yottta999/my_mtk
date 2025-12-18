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
