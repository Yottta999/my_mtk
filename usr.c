#include <stdio.h>
#include "mtk_c.h"

#define FIELD_WIDTH 30
#define FIELD_HEIGHT 40
#define CONTROLLER_SIZE 5

/* フィールド管理 */
typedef struct {
    char field[FIELD_HEIGHT][FIELD_WIDTH];
    int p1_x;
    int p1_y;
    int p2_x;
    int p2_y;
    int puck_x;
    int puck_y;
    int puck_vx;
    int puck_vy;
    int my_score;
    int opp_score;
    int is_player1;
    int update_display;
    int game_running;
    unsigned int rand_seed;
} GameState;

GameState game;

/* 簡易乱数生成器 */
int my_rand() {
    game.rand_seed = game.rand_seed * 1103515245 + 12345;
    return (game.rand_seed / 65536) % 32768;
}

void init_game(int is_p1) {
    game.is_player1 = is_p1;
    game.p1_x = FIELD_WIDTH / 2;
    game.p1_y = 3;
    game.p2_x = FIELD_WIDTH / 2;
    game.p2_y = FIELD_HEIGHT - 4;
    game.puck_x = FIELD_WIDTH / 2;
    game.puck_y = FIELD_HEIGHT / 2;
    game.my_score = 0;
    game.opp_score = 0;
    game.update_display = 1;
    game.game_running = 1;
    game.rand_seed = 12345;
    
    if (is_p1) {
        game.puck_vx = 0;
        game.puck_vy = 1;
    } else {
        game.puck_vx = 0;
        game.puck_vy = 0;
    }
}

/* フィールドを初期化 */
void clear_field() {
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            if (x == 0 || x == FIELD_WIDTH - 1) {
                game.field[y][x] = '|';
            } else if (y == 0 || y == FIELD_HEIGHT - 1) {
                game.field[y][x] = '-';
            } else if (y == FIELD_HEIGHT / 2) {
                game.field[y][x] = ':';
            } else {
                game.field[y][x] = ' ';
            }
        }
    }
}

/* フィールドに要素を配置 */
void update_field() {
    clear_field();
    
    /* プレイヤー1のコントローラを配置 */
    for (int dx = -CONTROLLER_SIZE/2; dx <= CONTROLLER_SIZE/2; dx++) {
        int x = game.p1_x + dx;
        if (x > 0 && x < FIELD_WIDTH - 1) {
            if (game.p1_y > 0 && game.p1_y < FIELD_HEIGHT - 1) {
                game.field[game.p1_y][x] = '1';
            }
        }
    }
    
    /* プレイヤー2のコントローラを配置 */
    for (int dx = -CONTROLLER_SIZE/2; dx <= CONTROLLER_SIZE/2; dx++) {
        int x = game.p2_x + dx;
        if (x > 0 && x < FIELD_WIDTH - 1) {
            if (game.p2_y > 0 && game.p2_y < FIELD_HEIGHT - 1) {
                game.field[game.p2_y][x] = '2';
            }
        }
    }
    
    /* パックを配置 */
    if (game.puck_x > 0 && game.puck_x < FIELD_WIDTH - 1 &&
        game.puck_y > 0 && game.puck_y < FIELD_HEIGHT - 1) {
        game.field[game.puck_y][game.puck_x] = 'O';
    }
}

/* 画面描画 */
void draw_game() {
    printf("\x1b[2J\x1b[H");
    
    if (game.is_player1) {
        printf("=== PLAYER 1 (TOP) ===\n");
        printf("YOU: %d | OPP: %d\n", game.my_score, game.opp_score);
    } else {
        printf("=== PLAYER 2 (BOTTOM) ===\n");
        printf("OPP: %d | YOU: %d\n", game.opp_score, game.my_score);
    }
    
    for (int y = 0; y < FIELD_HEIGHT; y++) {
        for (int x = 0; x < FIELD_WIDTH; x++) {
            printf("%c", game.field[y][x]);
        }
        printf("\n");
    }
    
    if (game.is_player1) {
        printf("Controls: a=LEFT d=RIGHT q=QUIT\n");
    } else {
        printf("Controls: a=LEFT d=RIGHT q=QUIT\n");
    }
}

/* パック物理演算 */
void update_puck() {
    game.puck_x += game.puck_vx;
    game.puck_y += game.puck_vy;
    
    /* 左右の壁で反射 */
    if (game.puck_x <= 1) {
        game.puck_x = 1;
        game.puck_vx = -game.puck_vx;
    }
    if (game.puck_x >= FIELD_WIDTH - 2) {
        game.puck_x = FIELD_WIDTH - 2;
        game.puck_vx = -game.puck_vx;
    }
    
    /* プレイヤー1のコントローラとの衝突 */
    if (game.puck_y == game.p1_y || game.puck_y == game.p1_y + 1) {
        int dx = game.puck_x - game.p1_x;
        if (dx >= -CONTROLLER_SIZE/2 && dx <= CONTROLLER_SIZE/2 && game.puck_vy < 0) {
            game.puck_vy = -game.puck_vy;
            /* 乱数で反射方向を決定 */
            int r = my_rand() % 5;
            if (r == 0) game.puck_vx = -1;
            else if (r == 1) game.puck_vx = 1;
            else if (r == 2) game.puck_vx = -2;
            else if (r == 3) game.puck_vx = 2;
            else game.puck_vx = 0;
        }
    }
    
    /* プレイヤー2のコントローラとの衝突 */
    if (game.puck_y == game.p2_y || game.puck_y == game.p2_y - 1) {
        int dx = game.puck_x - game.p2_x;
        if (dx >= -CONTROLLER_SIZE/2 && dx <= CONTROLLER_SIZE/2 && game.puck_vy > 0) {
            game.puck_vy = -game.puck_vy;
            /* 乱数で反射方向を決定 */
            int r = my_rand() % 5;
            if (r == 0) game.puck_vx = -1;
            else if (r == 1) game.puck_vx = 1;
            else if (r == 2) game.puck_vx = -2;
            else if (r == 3) game.puck_vx = 2;
            else game.puck_vx = 0;
        }
    }
    
    /* 得点判定 */
    if (game.puck_y <= 0) {
        if (game.is_player1) {
            game.opp_score++;
        } else {
            game.my_score++;
        }
        game.puck_x = FIELD_WIDTH / 2;
        game.puck_y = FIELD_HEIGHT / 2;
        game.puck_vx = 0;
        game.puck_vy = 1;
        game.update_display = 1;
    }
    
    if (game.puck_y >= FIELD_HEIGHT - 1) {
        if (game.is_player1) {
            game.my_score++;
        } else {
            game.opp_score++;
        }
        game.puck_x = FIELD_WIDTH / 2;
        game.puck_y = FIELD_HEIGHT / 2;
        game.puck_vx = 0;
        game.puck_vy = -1;
        game.update_display = 1;
    }
}

/* タスク1: キーボード入力処理 */
void keyboard_input_task() {
    char c[2];
    while (1) {
        fscanf(stdin, "%1s", c);
        
        P(1);
        if (game.is_player1) {
            if (c[0] == 'a' && game.p1_x > CONTROLLER_SIZE/2 + 1) {
                game.p1_x--;
                game.update_display = 1;
            } else if (c[0] == 'd' && game.p1_x < FIELD_WIDTH - CONTROLLER_SIZE/2 - 2) {
                game.p1_x++;
                game.update_display = 1;
            }
        } else {
            if (c[0] == 'a' && game.p2_x > CONTROLLER_SIZE/2 + 1) {
                game.p2_x--;
                game.update_display = 1;
            } else if (c[0] == 'd' && game.p2_x < FIELD_WIDTH - CONTROLLER_SIZE/2 - 2) {
                game.p2_x++;
                game.update_display = 1;
            }
        }
        
        if (c[0] == 'q') {
            game.game_running = 0;
        }
        V(1);
    }
}

/* タスク2: データ送信処理 */
void send_task() {
    while (1) {
        for (int i = 0; i < 100000; i++);
        
        P(1);
        if (game.is_player1) {
            fprintf(com0out, "P1%02d%02d%02d%02d%02d%02d%d%d\n",
                    game.p1_x, game.p1_y,
                    game.puck_x, game.puck_y,
                    game.puck_vx + 10, game.puck_vy + 10,
                    game.my_score, game.opp_score);
        } else {
            fprintf(com0out, "P2%02d%02d%02d%02d%02d%02d%d%d\n",
                    game.p2_x, game.p2_y,
                    game.puck_x, game.puck_y,
                    game.puck_vx + 10, game.puck_vy + 10,
                    game.my_score, game.opp_score);
        }
        V(1);
    }
}

/* タスク3: データ受信処理 */
void receive_task() {
    char buf[32];
    int player_id, cx, cy, px, py, pvx, pvy, s1, s2;
    
    while (1) {
        if (fscanf(com0in, "%s", buf) == 1) {
            if (buf[0] == 'P' && (buf[1] == '1' || buf[1] == '2')) {
                player_id = buf[1] - '0';
                
                if ((game.is_player1 && player_id == 2) || 
                    (!game.is_player1 && player_id == 1)) {
                    
                    cx = (buf[2] - '0') * 10 + (buf[3] - '0');
                    cy = (buf[4] - '0') * 10 + (buf[5] - '0');
                    px = (buf[6] - '0') * 10 + (buf[7] - '0');
                    py = (buf[8] - '0') * 10 + (buf[9] - '0');
                    pvx = (buf[10] - '0') * 10 + (buf[11] - '0') - 10;
                    pvy = (buf[12] - '0') * 10 + (buf[13] - '0') - 10;
                    s1 = buf[14] - '0';
                    s2 = buf[15] - '0';
                    
                    P(1);
                    if (game.is_player1) {
                        game.p2_x = cx;
                        game.p2_y = cy;
                    } else {
                        game.p1_x = cx;
                        game.p1_y = cy;
                        game.puck_x = px;
                        game.puck_y = py;
                        game.puck_vx = pvx;
                        game.puck_vy = pvy;
                    }
                    game.update_display = 1;
                    V(1);
                }
            }
        }
    }
}

/* タスク4: パック物理演算処理（Player1のみ） */
void physics_task() {
    while (1) {
        for (int i = 0; i < 80000; i++);
        
        P(1);
        if (game.is_player1 && game.game_running) {
            update_puck();
            game.update_display = 1;
        }
        V(1);
    }
}

/* タスク5: 画面描画処理 */
void display_task() {
    while (1) {
        for (int i = 0; i < 150000; i++);
        
        P(1);
        if (game.update_display) {
            update_field();
            draw_game();
            game.update_display = 0;
        }
        
        if (!game.game_running) {
            printf("\n=== GAME OVER ===\n");
            printf("Final Score - ");
            if (game.is_player1) {
                printf("YOU: %d | OPP: %d\n", game.my_score, game.opp_score);
            } else {
                printf("OPP: %d | YOU: %d\n", game.opp_score, game.my_score);
            }
            V(1);
            while(1);
        }
        V(1);
    }
}

/* タスク6: ゲーム状態監視 */
void monitor_task() {
    int prev_my_score = 0;
    int prev_opp_score = 0;
    
    while (1) {
        for (int i = 0; i < 200000; i++);
        
        P(1);
        if (game.my_score != prev_my_score || game.opp_score != prev_opp_score) {
            prev_my_score = game.my_score;
            prev_opp_score = game.opp_score;
            game.update_display = 1;
        }
        V(1);
    }
}

int main() {
    char choice[2];
    
    printf("=============================\n");
    printf("   HOCKEY GAME - VERTICAL\n");
    printf("=============================\n");
    printf("Are you Player 1 or 2? (1/2): ");
    
    fd_mapping();
    init_kernel();
    
    scanf("%1s", choice);
    
    if (choice[0] == '1') {
        init_game(1);
        printf("You are Player 1 (TOP side)\n");
        printf("Controller marked as '1'\n");
    } else {
        init_game(0);
        printf("You are Player 2 (BOTTOM side)\n");
        printf("Controller marked as '2'\n");
    }
    
    printf("Starting game...\n");
    
    for (int i = 0; i < 2000000; i++);
    
    /* タスク登録 */
    set_task(keyboard_input_task);  /* タスク1: キーボード入力 */
    set_task(send_task);            /* タスク2: データ送信 */
    set_task(receive_task);         /* タスク3: データ受信 */
    set_task(physics_task);         /* タスク4: 物理演算 */
    set_task(display_task);         /* タスク5: 画面描画 */
    set_task(monitor_task);         /* タスク6: 状態監視 */
    
    update_field();
    draw_game();
    
    begin_sch();
    
    return 0;
}
