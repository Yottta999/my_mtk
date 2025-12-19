/* ============================================
   テトリス用アセンブリ補助関数
   ============================================ */

.include "defs.s"

.section .text

/* ============================================
   GETSTRING ラッパー
   int GETSTRING(int ch, char *buf, int size)
   ============================================ */
.global GETSTRING
.even
GETSTRING:
    movem.l %D2-%D3, -(%SP)
    
    /* 引数取得 */
    move.l 16(%SP), %D1    /* ch */
    move.l 20(%SP), %D2    /* buf */
    move.l 24(%SP), %D3    /* size */
    
    /* システムコール */
    move.l #SYSCALL_NUM_GETSTRING, %D0
    trap #0
    
    /* 戻り値は既に %D0 に */
    movem.l (%SP)+, %D2-%D3
    rts

/* ============================================
   PUTSTRING ラッパー
   int PUTSTRING(int ch, char *buf, int size)
   ============================================ */
.global PUTSTRING
.even
PUTSTRING:
    movem.l %D2-%D3, -(%SP)
    
    /* 引数取得 */
    move.l 16(%SP), %D1    /* ch */
    move.l 20(%SP), %D2    /* buf */
    move.l 24(%SP), %D3    /* size */
    
    /* システムコール */
    move.l #SYSCALL_NUM_PUTSTRING, %D0
    trap #0
    
    movem.l (%SP)+, %D2-%D3
    rts

/* ============================================
   P操作（セマフォ取得）
   void P(int sem_id)
   ============================================ */
.global P
.even
P:
    move.l 4(%SP), %D0     /* sem_id */
    trap #1                /* PV handler */
    rts

/* ============================================
   V操作（セマフォ解放）
   void V(int sem_id)
   ============================================ */
.global V
.even
V:
    move.l 4(%SP), %D0     /* sem_id */
    addi.l #0x10000, %D0   /* V操作フラグ */
    trap #1
    rts

/* ============================================
   ランダム数生成（簡易版）
   int rand(void)
   ============================================ */
.section .data
rand_seed: .long 12345

.section .text
.global rand
.even
rand:
    /* 線形合同法: seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF */
    move.l rand_seed, %D0
    mulu.l #1103515245, %D0
    addi.l #12345, %D0
    andi.l #0x7FFFFFFF, %D0
    move.l %D0, rand_seed
    rts

/* ============================================
   メモリコピー
   void memcpy(void *dst, void *src, int n)
   ============================================ */
.global memcpy
.even
memcpy:
    movem.l %A0-%A1, -(%SP)
    
    movea.l 12(%SP), %A0   /* dst */
    movea.l 16(%SP), %A1   /* src */
    move.l 20(%SP), %D0    /* n */
    
    subq.l #1, %D0
    bmi memcpy_end
    
memcpy_loop:
    move.b (%A1)+, (%A0)+
    dbra %D0, memcpy_loop
    
memcpy_end:
    movem.l (%SP)+, %A0-%A1
    rts

/* ============================================
   メモリセット
   void memset(void *ptr, int value, int n)
   ============================================ */
.global memset
.even
memset:
    movem.l %A0, -(%SP)
    
    movea.l 8(%SP), %A0    /* ptr */
    move.l 12(%SP), %D1    /* value */
    move.l 16(%SP), %D0    /* n */
    
    subq.l #1, %D0
    bmi memset_end
    
memset_loop:
    move.b %D1, (%A0)+
    dbra %D0, memset_loop
    
memset_end:
    movem.l (%SP)+, %A0
    rts

/* ============================================
   ブロック回転処理（高速化版）
   void rotate_matrix_90(int src[4][4], int dst[4][4])
   ============================================ */
.global rotate_matrix_90
.even
rotate_matrix_90:
    movem.l %A0-%A1/%D0-%D4, -(%SP)
    
    movea.l 24(%SP), %A0   /* src */
    movea.l 28(%SP), %A1   /* dst */
    
    moveq #0, %D2          /* y */
rotate_y_loop:
    moveq #0, %D3          /* x */
rotate_x_loop:
    /* dst[x][3-y] = src[y][x] */
    /* src_offset = y * 16 + x * 4 */
    move.l %D2, %D0
    lsl.l #4, %D0          /* y * 16 */
    move.l %D3, %D1
    lsl.l #2, %D1          /* x * 4 */
    add.l %D1, %D0
    move.l (%A0, %D0.l), %D4
    
    /* dst_offset = x * 16 + (3-y) * 4 */
    move.l %D3, %D0
    lsl.l #4, %D0          /* x * 16 */
    moveq #3, %D1
    sub.l %D2, %D1         /* 3 - y */
    lsl.l #2, %D1          /* (3-y) * 4 */
    add.l %D1, %D0
    move.l %D4, (%A1, %D0.l)
    
    addq.l #1, %D3
    cmpi.l #4, %D3
    blt rotate_x_loop
    
    addq.l #1, %D2
    cmpi.l #4, %D2
    blt rotate_y_loop
    
    movem.l (%SP)+, %A0-%A1/%D0-%D4
    rts

/* ============================================
   高速衝突判定（ビット演算版）
   int fast_collision_check(int field_row, int block_row)
   戻り値: 0=衝突なし, 1=衝突あり
   ============================================ */
.global fast_collision_check
.even
fast_collision_check:
    move.l 4(%SP), %D0     /* field_row */
    move.l 8(%SP), %D1     /* block_row */
    
    and.l %D1, %D0         /* ビットAND */
    sne.b %D0              /* 結果が0でなければ1 */
    andi.l #1, %D0
    rts
