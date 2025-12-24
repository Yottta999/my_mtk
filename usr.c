#include <stdio.h>
#include <stdlib.h>
#include "mtk_c.h"

/* --- ハードウェア定義 (環境に合わせて調整) --- */
#define REGBASE 0xfff000
volatile unsigned short *USTCNT1 = (unsigned short *)(REGBASE + 0x900);
volatile unsigned short *URX1    = (unsigned short *)(REGBASE + 0x904);
volatile unsigned short *USTCNT2 = (unsigned short *)(REGBASE + 0x940); // ※要確認
volatile unsigned short *URX2    = (unsigned short *)(REGBASE + 0x944); // ※要確認

/* --- ゲーム設定 --- */
#define WIDTH  20
#define HEIGHT 15
#define PADDLE_W 3
#define GOAL_SCORE 3

/* キー定義 */
#define KEY_SWITCH ' '  // 方向転換
#define KEY_RESET  'r'  // 強制リセット

/* ANSI エスケープシーケンス */
#define ESC "\x1b"
#define CLS "\x1b[2J"
#define HIDE_CURSOR "\x1b[?25l"
#define COLOR_RESET "\x1b[0m"
#define COLOR_BALL  "\x1b[33m" // 黄
#define COLOR_P1    "\x1b[36m" // シアン
#define COLOR_P2    "\x1b[35m" // マゼンタ
#define COLOR_WALL  "\x1b[37m" // 白

/* グローバルゲーム状態 */
typedef struct {
    int x, y;           // ボール位置
    int vx, vy;         // ボール速度
    int old_x, old_y;   // 前回位置（消去用）
} Ball;

typedef struct {
    int x;              // パドル左端位置 (yは固定)
    int dir;            // 移動方向 (1:右, -1:左)
    int old_x;          // 前回位置
    int score;
} Paddle;

/* 共有データ */
Ball ball;
Paddle p1, p2; // P1は y=HEIGHT-2, P2は y=1
int game_state; // 0:待機, 1:プレイ中, 2:終了
int hit_count;  // 反射回数（速度調整用）
int winner;

/* --- 関数: 入出力 --- */

/* 座標移動 (P2は180度回転して出力) */
void term_goto(FILE *fp, int id, int x, int y) {
    if (id == 0) {
        // P1: 通常視点
        fprintf(fp, "\x1b[%d;%dH", y + 1, x + 1);
    } else {
        // P2: 反転視点 (xもyも逆)
        fprintf(fp, "\x1b[%d;%dH", (HEIGHT - 1 - y) + 1, (WIDTH - 1 - x) + 1);
    }
}

/* 文字描画 */
void term_put(FILE *fp, int id, int x, int y, char *c) {
    term_goto(fp, id, x, y);
    fprintf(fp, "%s", c);
} 

/* --- ゲームロジック --- */

void init_round() {
    ball.x = WIDTH / 2;
    ball.y = HEIGHT / 2;
    ball.vx = (rand() % 2 == 0) ? 1 : -1;
    ball.vy = (hit_count % 2 == 0) ? 1 : -1;
    ball.old_x = ball.x;
    ball.old_y = ball.y;

    p1.x = WIDTH / 2 - 1; p1.dir = 1; p1.old_x = p1.x;
    p2.x = WIDTH / 2 - 1; p2.dir = 1; p2.old_x = p2.x;
    
    hit_count = 0;
}

void draw_frame_static(FILE *fp, int id) {
    fprintf(fp, CLS);
    fprintf(fp, HIDE_CURSOR);
    
    // 枠描画
    term_put(fp, id, 0, 0, COLOR_WALL, "+");
    for(int i=1; i<WIDTH-1; i++) fprintf(fp, "-");
    fprintf(fp, "+");

    // スコア表示
    term_goto(fp, id, 2, HEIGHT+1); // 画面外下部
    fprintf(fp, COLOR_RESET "1P: %d  2P: %d", p1.score, p2.score);
}

/* 差分描画システム*/
void update_view(int id) {
    FILE *fp = (id == 0) ? com0out : com1out;
    
    // 1. ボールの消去と描画
    if (ball.x != ball.old_x || ball.y != ball.old_y) {
        term_put(fp, id, ball.old_x, ball.old_y, COLOR_RESET, " "); // 消す
        term_put(fp, id, ball.x, ball.y, COLOR_BALL, "O");          // 描く
    }

    // 2. パドルの消去と描画 (P1)
    if (p1.x != p1.old_x) {
        // P1は論理座標 y = HEIGHT - 2
        for(int i=0; i<PADDLE_W; i++) 
            term_put(fp, id, p1.old_x + i, HEIGHT - 2, COLOR_RESET, " ");
        for(int i=0; i<PADDLE_W; i++) 
            term_put(fp, id, p1.x + i, HEIGHT - 2, (id==0)?COLOR_P1:COLOR_P2, "=");
    }

    // 3. パドルの消去と描画 (P2)
    if (p2.x != p2.old_x) {
        // P2は論理座標 y = 1
        for(int i=0; i<PADDLE_W; i++) 
            term_put(fp, id, p2.old_x + i, 1, COLOR_RESET, " ");
        for(int i=0; i<PADDLE_W; i++) 
            term_put(fp, id, p2.x + i, 1, (id==1)?COLOR_P1:COLOR_P2, "=");
    }
}

void move_paddle(Paddle *p) {
    p->old_x = p->x;
    p->x += p->dir;
    
    // 壁反射
    if (p->x < 1) {
        p->x = 1;
        p->dir = 1; // 強制反転
    }
    if (p->x > WIDTH - 1 - PADDLE_W) {
        p->x = WIDTH - 1 - PADDLE_W;
        p->dir = -1;
    }
}

void update_physics() {
    ball.old_x = ball.x;
    ball.old_y = ball.y;
    
    ball.x += ball.vx;
    ball.y += ball.vy;

    // 壁反射 (左右)
    if (ball.x <= 0 || ball.x >= WIDTH - 1) {
        ball.vx = -ball.vx;
        ball.x += ball.vx; // めり込み防止
    }

    // P2パドル判定 (y=1付近)
    if (ball.y == 1) {
        if (ball.x >= p2.x && ball.x < p2.x + PADDLE_W) {
            ball.vy = 1; // 下へ弾く
            hit_count++;
            // ランダム反射
            ball.vx = (rand() % 3) - 1; // -1, 0, 1
            if (ball.vx == 0) ball.vx = (rand()%2)?1:-1; // 0回避
        }
    }
    
    // P1パドル判定 (y=HEIGHT-2付近)
    if (ball.y == HEIGHT - 2) {
        if (ball.x >= p1.x && ball.x < p1.x + PADDLE_W) {
            ball.vy = -1; // 上へ弾く
            hit_count++;
            ball.vx = (rand() % 3) - 1;
            if (ball.vx == 0) ball.vx = (rand()%2)?1:-1;
        }
    }

    // 得点判定
    if (ball.y <= 0) {
        p1.score++; // P2が落とした
        init_round();
        draw_frame_static(com0out, 0); // 画面リフレッシュ
        draw_frame_static(com1out, 1);
    }
    else if (ball.y >= HEIGHT - 1) {
        p2.score++; // P1が落とした
        init_round();
        draw_frame_static(com0out, 0);
        draw_frame_static(com1out, 1);
    }
}

/* メインタスク (ゲーム管理) */
void task1() {
    // コンソール初期化
    game_state = 0;
    p1.score = 0; p2.score = 0;
    
    fprintf(com0out, CLS "PRESS SPACE TO START\n");
    fprintf(com1out, CLS "PRESS SPACE TO START\n");

    while(1) {
        // --- 入力処理 (ノンブロッキング) ---
        char c1 = inkey(0);
        char c2 = inkey(1);

        if (game_state == 0) {
            // 待機画面
            if (c1 == KEY_SWITCH || c2 == KEY_SWITCH) {
                game_state = 1;
                init_round();
                draw_frame_static(com0out, 0);
                draw_frame_static(com1out, 1);
            }
            continue;
        }
        else if (game_state == 2) {
            // 勝利画面
            if (c1 == KEY_SWITCH || c2 == KEY_SWITCH) {
                p1.score = 0; p2.score = 0;
                game_state = 0;
                fprintf(com0out, CLS "RESTART? PRESS SPACE\n");
                fprintf(com1out, CLS "RESTART? PRESS SPACE\n");
            }
            continue;
        }
        // 勝敗判定
        if (p1.score >= GOAL_SCORE || p2.score >= GOAL_SCORE) {
            game_state = 2;
            winner = (p1.score >= GOAL_SCORE) ? 1 : 2;
            
            fprintf(com0out, CLS "\x1b[10;10H %s \n", (winner==1)?"YOU WIN!":"YOU LOSE");
            fprintf(com1out, CLS "\x1b[10;10H %s \n", (winner==2)?"YOU WIN!":"YOU LOSE");
        }
    }
}

void task2() {
    while(1){
        char c1 = inkey(0);
        if (c1 == KEY_SWITCH) p1.dir *= -1;
        move_paddle(&p1);
    }
}

void task3() {
    while(1){
        char c2 = inkey(1);
        if (c2 == KEY_SWITCH) p2.dir *= -1;
        move_paddle(&p2);
    }
}

void task4() {
    while(1){
        if(game_state == 1){
            update_physics();
            update_view(0);
            update_view(1);
        }
        int wait = 150 - (hit_count * 5);
        if (wait < 30) wait = 30; // 最速リミット
        for (int i = 0; i < wait*10000; i++);
    }
}


/* main.c の代わり */
int main() {
    init_kernel();
    fd_mapping();
    set_task(task1);
    set_task(task2);
    set_task(task3);
    set_task(task4);

    begin_sch();
    return 0;
}
