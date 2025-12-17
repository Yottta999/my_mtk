.include "defs.s"

/* スタック */
.section .bss
.even
SYS_STK:.ds.b 0x4000
.even
SYS_STK_TOP: | システムスタック領域の最後尾

/* 初期化 */
.section .text
.even
.extern start
.global monitor_begin
monitor_begin:
boot:
    move.w #0x2700, %SR     | 割り込み禁止
    lea.l SYS_STK_TOP, %SP  | スタックポインタの設定

    /* 割り込みコントローラの初期化 */
    move.b #0x40, IVR       | ユーザ割り込みベクタ番号を0x40*4(=0x100)+levelに設定
    move.l #0x00ffffff, IMR | 全割り込みマスク

    move.l #syscall_handler, 0x080 | TRAP#0の割り込みベクタを登録
    move.l #uart1_interrupt, 0x110 | UART1の割り込みベクタを登録
    move.l #uart2_interrupt, 0x114 | UART2の割り込みベクタを登録  
    move.l #tmr1_interrupt, 0x118  | TIMER1の割り込みベクタを登録

    /* 送受信 (UART1) 関係の初期化 (割り込みレベルは 4 に固定されている) */
    move.w #0x0000, USTCNT1   | リセット
    * move.w #0xe100, USTCNT1 | 送受信可能, パリティなし, 1 stop, 8 bit, 送受割り込み禁止
    move.w #0xe108, USTCNT1   | 受信割り込み可能
    * move.w #0xe104, USTCNT1 | 送信割り込み可能
    move.w #0x0038, UBAUD1    | baud rate = 230400 bps
    
    /* 送受信 (UART2) 関係の初期化 (割り込みレベルは 4 に固定されている) */
    move.w #0x0000, USTCNT2    | リセット
    * move.w #0xe100, USTCNT2  | 送受信可能, パリティなし, 1 stop, 8 bit, 送受割り込み禁止
    move.w #0xe108, USTCNT2    | 受信割り込み可能
    * move.w #0xe104, USTCNT2  | 送信割り込み可能
    move.w #0x0038, UBAUD2     | baud rate = 230400 bps

    /* タイマ関係の初期化 (割り込みレベルは 6 に固定されている) */
    move.w #0x0004, TCTL1   | restart, 割り込み不可,
                            | システムクロックの 1/16 を単位として計時，
                            | タイマ使用停止

    jsr	Init_Q            | キューの初期化

    * move.l #0xff3ffb, IMR | UART1の割り込みを許可
    * move.l #0xff3ff9, IMR | UART1,TIMERの割り込みを許可
    move.l #0xff2ff9, IMR
    move.w #0x2000, %SR   | スーパーバイザモード・走行レベルは0
    jmp start
    * bra MAIN


****************************************************************
*** 初期値の無いデータ領域
****************************************************************
.section .bss
BUF:
    .ds.b 256 | BUF[256]
    .even
USR_STK:
    .ds.b 0x4000 | ユーザスタック領域
    .even
USR_STK_TOP:     | ユーザスタック領域の最後尾
******************************************************************
/*; ---- end include main.s ----*/


/* 割り込みハンドラ */
/* ---- begin include syscall.s ---- */

.section .text
/*D0の値で呼び出すサブルーチンを決めている。ここでは何を呼び出しているかを分かりやすくするためにシンボルに数値を定義している(室原)*/
.equ SYSCALL_NUM_GETSTRING,   1 |文字列入力（GETSTRING）（ここからコメント鴻上）|
.equ SYSCALL_NUM_PUTSTRING,   2 |文字列出力(PUTSTRING)|
.equ SYSCALL_NUM_RESET_TIMER, 3 |タイマリセット|
.equ SYSCALL_NUM_SET_TIMER,   4 |タイマセット|
.equ SYSCALL_NUM_SKIPMT,      5 |SKIPMT
/*
 * syscall_handler
 * %d0: syscall number
 * %dx: syscall argument
	*/
******************************************
**syscall_handlerはTRAP #0により呼びだされるシステムコール共通ハンドラ（鴻上）
**D0レジスタの値によってどのサブルーチンをよぶかを分岐する
**入力D0.l :システムコール番号（上記定義のいづれか）
**D1~D7/A0~A6:各システムコールに応じた引数
**出力
**必要に応じてD0に戻り値を格納（鴻上）
*****************************************	
	
syscall_handler:
        cmpi.l #SYSCALL_NUM_SKIPMT, %D0
        beq tmr1_interrupt

	movem.l %D1-%D7/%A0-%A6, -(%SP)
	cmpi.l #SYSCALL_NUM_GETSTRING, %D0   |D0==1?（鴻上）|
	beq CALL_GETSTRING                   |→GETSTRING処理へ|

	cmpi.l #SYSCALL_NUM_PUTSTRING, %D0   |D0==2?|
	beq CALL_PUTSTRING                   |→PUTSTRING処理へ|
	cmpi.l #SYSCALL_NUM_RESET_TIMER, %D0 |D0==3?|
	beq CALL_RESET_TIMER                 |RESET_TIMER処理へ|
	cmpi.l #SYSCALL_NUM_SET_TIMER, %D0   |D0==4?|
	beq CALL_SET_TIMER                   |CALL_SET_TIMER処理へ|
**********************
** いずれのシステムコール番号にも該当しない
**********************	
END_SYSCALL_HNDR:
    movem.l (%SP)+, %D1-%D7/%A0-%A6
	rte
**********
**各システムコール処理の分岐先
*********	
/*各サブルーチンを呼び出すための場所、サブルーチン呼出し後はシステムコールハンドラーの終了処理に移る(室原)*/	
CALL_GETSTRING:
	jsr GETSTRING
	bra END_SYSCALL_HNDR

CALL_PUTSTRING:
	jsr PUTSTRING
	bra END_SYSCALL_HNDR
	
CALL_RESET_TIMER:
	jsr RESET_TIMER
	bra END_SYSCALL_HNDR
CALL_SET_TIMER:
	jsr SET_TIMER
	bra END_SYSCALL_HNDR
/*; ---- end include syscall.s ----*/

/* ---- begin include queue.s ---- */
.section .text


******INTERPUT*************************************************************************************
** 入力：d1.l（チャンネル）
** チャネル ch の送信キューからデータを一つ取り出し，実際に送信	
INTERPUT:
	movem.l %d0-%d2,-(%sp)
	
	/* (1) */
	move.w  #0x2700, %sr
	/* (2) */
	cmpi.l #0, %d1
	beq INTERPUT0
	cmpi.l #1, %d1
	beq INTERPUT1
	bra End_INTERPUT		|ch=0,1以外なら何もしない
INTERPUT0:	
	/* (3) */
	moveq	#1, %d0
	jsr	OUTQ			|送信キューからデータを一つ取り出し
	*出力１：d0(失敗 0/ 成功 1 )
	*出力２：d1（取り出した8bitデータ）
	/* (4) */
	cmpi #0, %d0
	beq INTERPUT_MASK0		|取り出し失敗なら、送信割り込みをマスク（禁止）
	/* (5) */
	/* d1をUTX1に代入（下位８bit） */
	ori #0x0800, %D1                | ヘッダを代入
	move.w %D1, UTX1                | 送信
	bra End_INTERPUT
INTERPUT_MASK0:
	/* (4)' */		|送信失敗した場合、送信割り込み禁止
	move.w	USTCNT1, %d2
	andi.w	#0xFFFB, %d2	| 0xFFFB = 1111111111111011 
	move.w	%d2, USTCNT1    | USTCNT1のTXEEを0にする 
	bra End_INTERPUT
INTERPUT1:	
	/* (3) */
	moveq	#3, %d0
	jsr	OUTQ			|送信キューからデータを一つ取り出し
	*出力１：d0(失敗 0/ 成功 1 )
	*出力２：d1（取り出した8bitデータ）
	/* (4) */
	cmpi #0, %d0
	beq	INTERPUT_MASK1		|取り出し失敗なら、送信割り込みをマスク（禁止）
	/* (5) */
	/* d1をUTX2に代入（下位８bit） */
	ori #0x0800, %D1                | ヘッダを代入
	move.w %D1, UTX2                | 送信
	bra	End_INTERPUT
INTERPUT_MASK1:
	/* (4)' */		|送信失敗した場合、送信割り込み禁止
	move.w	USTCNT2, %d2
	andi.w	#0xFFFB, %d2	| 0xFFFB = 1111111111111011 
	move.w	%d2, USTCNT2    | USTCNT2のTXEEを0にする 
End_INTERPUT:
	movem.l (%sp)+, %d0-%d2
	rts

	
******************************************************************************************************	

*********PUTSTRING************************************************************************************
** データを送信キューに格納し，送信割り込みを開始	
** 入力１：チャネル ch → %D1.L
** 入力２：データ読み込み先の先頭アドレス p → %D2.L
** 入力３：送信するデータ数 size → %D3.L
** 出力  ：実際に送信したデータ数 sz → %D0.L
	
PUTSTRING:
	movem.l %d1-%d6/%a0,-(%sp)
	/* (1) */
	cmpi.l #0, %d1		
	beq PUTSTRING0
	cmpi.l #1, %d1
	beq PUTSTRING1
	bra	End_PUTSTRING	| ch=0,1以外なら何もしない
PUTSTRING0:
	/* (2) */
	moveq	#0, %d4  	| d4 = sz = 0 
	movea.l	%d2, %a0 	| a0 = i = p 
	/* (3) */
	cmpi 	#0, %d3		| 送信サイズが０のとき
	beq	PUTSTRING0_10
LOOP_PUTSTRING0:	
	/* (4) */
	cmp	%d3, %d4	| 送信サイズ == 送信した数？
	beq	PUTSTRING0_9
	/* (5) */
	moveq	#1, %d0
	move.b	(%a0), %d1	| a0:データ読み込み先の先頭アドレス
	jsr	INQ
	*d0 :キュー番号
	*d1 :8bitデータ
	*出力：d0(失敗 0/ 成功 1 )
	/* (6) */
	cmpi 	#0, %d0		| INQ失敗？
	beq	PUTSTRING0_9	
	/* (7) */
	addq	#1, %d4		| 送信した数に１を足す
	addq	#1, %a0		| 読み込むデータを次に
	/* (8) */
	bra 	LOOP_PUTSTRING0	| 全て送信 or INQ失敗まで続ける
PUTSTRING0_9:
	/* (9) */
	move.w	USTCNT1, %d6
	ori.w	#0x0004, %d6	| 0xFFFB = 0000000000000100 
	move.w	%d6,	USTCNT1 | 送信割込み許可 
PUTSTRING0_10:
	/* (10) */
	move.l	%d4, %d0	| d0(出力) = sz(送信した数)
	bra End_PUTSTRING
	
PUTSTRING1:
	/* (2) */
	moveq	#0, %d4  	| d4 = sz = 0 
	movea.l	%d2, %a0 	| a0 = i = p 
	/* (3) */
	cmpi 	#0, %d3		| 送信サイズが０のとき
	beq	PUTSTRING1_10
LOOP_PUTSTRING1:	
	/* (4) */
	cmp	%d3, %d4	| 送信サイズ == 送信した数？
	beq	PUTSTRING1_9
	/* (5) */
	moveq	#3, %d0
	move.b	(%a0), %d1	| a0:データ読み込み先の先頭アドレス
	jsr	INQ
	*d0 :キュー番号
	*d1 :8bitデータ
	*出力：d0(失敗 0/ 成功 1 )
	/* (6) */
	cmpi 	#0, %d0		| INQ失敗？
	beq	PUTSTRING1_9	
	/* (7) */
	addq	#1, %d4		| 送信した数に１を足す
	addq	#1, %a0		| 読み込むデータを次に
	/* (8) */
	bra 	LOOP_PUTSTRING1	| 全て送信 or INQ失敗まで続ける
PUTSTRING1_9:
	/* (9) */
	move.w	USTCNT2, %d6
	ori.w	#0x0004, %d6	| 0xFFFB = 0000000000000100 
	move.w	%d6, USTCNT2    | 送信割込み許可 
PUTSTRING1_10:
	/* (10) */
	move.l	%d4, %d0	| d0(出力) = sz(送信した数)
End_PUTSTRING:
	movem.l (%sp)+, %d1-%d6/%a0
	rts
	
******************************************************************************************************
*****************************
** キュー構造体定義
******************************
.equ Q_SIZE, 256              
.equ Q_COUNT, 4               
.equ top_ofs,     Q_SIZE     
.equ bottom_ofs,  Q_SIZE+4
.equ out_ofs,     Q_SIZE+8
.equ in_ofs,      Q_SIZE+12
.equ s_ofs,       Q_SIZE+16
.equ Q_STRIDE,    Q_SIZE+20 |キュー構造体の全長
**********************
** キューの初期化処理
**********************
Init_Q:
	movem.l	%d0-%d1/%a0-%a1,-(%sp)
	moveq #Q_COUNT, %d0
	subq #1, %d0
	lea.l QUEUE0, %a0 | %a0 = QUEUE0の開始アドレス
	move.l #Q_SIZE, %d1
	subq #1, %d1      | %d1 = #255

Init_Q_LOOP:
	movea.l %a0, %a1
	adda.l %d1, %a1   | %a1 = QUEUEno の末尾アドレス
	move.l %a0, (%a0, top_ofs)
	move.l %a0, (%a0, out_ofs)
	move.l %a0, (%a0, in_ofs)
	move.l %a1, (%a0, bottom_ofs)
	move.l #0, (%a0, s_ofs)
	
	adda.l #Q_STRIDE, %a0
	dbra   %d0, Init_Q_LOOP

	movem.l	(%sp)+,%d0-%d1/%a0-%a1
	rts
	
***********************************
** INQ: キューへのデータ書き込み
** 入力 d0.l: キューインデックス
**      d1.b: 書き込むデータ
** 出力 d0.l: 結果(00:失敗, 00以外:成功)
***********************************
INQ:
	movem.l	%d1-%d2/%a0-%a2,-(%sp)
	move.w	%sr,-(%sp)
	move.w	#0x2700,%sr | 走行レベル７
	jsr	PUT_BUF
	move.w	(%sp)+,%sr  | 旧走行レベルの回復
	movem.l	(%sp)+,%d1-%d2/%a0-%a2
	rts

****************************************
** PUT_BUF
****************************************
PUT_BUF:
	mulu #Q_STRIDE, %d0
	lea.l QUEUE0, %a0
	adda.l %d0, %a0   |%a0 = キューno の先頭アドレス

	move.l (%a0,s_ofs), %d2
	cmpi.l #Q_SIZE, %d2
	beq PUT_BUF_FALSE
	
	movea.l (%a0, in_ofs), %a1
	move.b %d1,(%a1)+

	move.l (%a0, bottom_ofs), %a2    | %a2 = キュー終端アドレス
	cmpa.l %a2, %a1
	bls PUT_BUF_STEP1           | in がキューの終端をこえたら
	move.l %a0, %a1             | %a1 = キューnoの先頭アドレス
	
PUT_BUF_STEP1:	
	move.l %a1, (%a0, in_ofs)
	addq.l #1, (%a0, s_ofs)
	moveq.l #1, %d0
	bra PUT_BUF_end

PUT_BUF_FALSE:
	move.l	#0, %d0

PUT_BUF_end:
	rts

***********************************
** OUTQ : キューへのデータ書き込み
** 入力 d0.l : キュー番号
** 出力 d0.l : 結果(00:失敗, 00以外:成功)
***********************************
OUTQ:
	movem.l	%d2/%a0-%a2,-(%sp)
	move.w	%sr,-(%sp)
	move.w	#0x2700,%sr
	jsr	GET_BUF
	move.w	(%sp)+,%sr
	movem.l	(%sp)+,%d2/%a0-%a2
	rts

****************************************
** GET_BUF
****************************************
GET_BUF:
	moveq.l #0, %d1
	mulu #Q_STRIDE, %d0
	lea.l QUEUE0, %a0
	adda.l %d0, %a0   /* %a0 = 先頭 */

	move.l (%a0,s_ofs), %d2
	cmpi.l #0, %d2
	beq GET_BUF_FALSE
	
	movea.l (%a0, out_ofs), %a1
	move.b (%a1), %d1
	move.b #0, (%a1)+

	move.l (%a0, bottom_ofs), %a2    /* %a2 = キュー終端アドレス */
	cmpa.l %a2, %a1
	bls GET_BUF_STEP1           /* 終端に到達したら */
	move.l %a0, %a1              /* %a1 = キュー先頭アドレス */
	
GET_BUF_STEP1:	
	move.l %a1, (%a0, out_ofs)
	subq.l   #1, (%a0, s_ofs)
	moveq.l #1, %d0
	bra GET_BUF_end

GET_BUF_FALSE:
	move.l	#0,%d0

GET_BUF_end:
	rts

******************************
** キュー用のメモリ領域確保
******************************
.section .data
.even
QUEUE0: .ds.b Q_STRIDE
QUEUE1: .ds.b Q_STRIDE
QUEUE2: .ds.b Q_STRIDE
QUEUE3: .ds.b Q_STRIDE

.section .text
.even
***************************
** %d1.l = ch
** %d2.b = 受信データ  data
** 戻り値  なし
***************************
/* 受信レジスタの値はすでにd2に格納されている。あとは送信キューに入れるだけ。（河野） */	
INTERGET:
	cmpi.l #0, %d1    /* ch != 0,1 なら終了 */
	beq INTERGET0
	cmpi.l #1, %d1
	beq INTERGET1                 
	bra INTERGET_END 
INTERGET0:	
        move.b %d2, %d1   /* %d1 = data */
	moveq.l #0, %d0   /* キュー番号を 0 に設定 */          
	jsr INQ           /* INQ(0, data) */
	bra INTERGET_END
INTERGET1:
        move.b %d2, %d1   /* %d1 = data */
	moveq.l #2, %d0   /* キュー番号を 2 に設定 */          
	jsr INQ           /* INQ(2, data) */
INTERGET_END:
	rts
	
**********************************************
** %d1.l = ch
** %d2.l = 書き込み先の先頭アドレス p
** %d3.l = 取り出すデータ数  size
** 戻り値  %d0.l = 実際に取り出したデータ数  sz 
**********************************************
/* 受信キューにあるsize個のデータを、p番地に書き込み（河野） */	
GETSTRING:
	movem.l %d4/%a0, -(%sp)
	cmp.l #0, %d1            /* ch != 0,1 なら終了 */
	beq GETSTRING0
	cmp.l #0, %d1       
	beq GETSTRING1       
	bra GETSTRING_END
GETSTRING0:
	moveq.l #0, %d4          /* %d4 = 0 , %d4は%d0(=sz)の一時保存レジスタ */                    
	movea.l %d2, %a0         /* %a0 = p */       

GETSTRING0_STEP1:
	cmp.l %d4, %d3           /* %d4 = size なら STEP2 へ */               
	beq GETSTRING0_STEP2
	moveq.l #0, %d0          /* %d0 = 0, キュー番号を0に */
	jsr OUTQ                 /* OUTQ(0,data) */      
	cmpi.l #0, %d0           /* 復帰値 %d0 が 0 なら STEP2 へ */           
	beq GETSTRING0_STEP2      
	move.b %d1, (%a0)+       /* %a0 番地へ data をコピー, %a0++ */                               
	addq.l #1, %d4           /* %d4++ */      
	bra GETSTRING0_STEP1      

GETSTRING0_STEP2:
	move.l %d4, %d0          /* %d0 = %d4 */  

GETSTRING1:
	moveq.l #0, %d4          /* %d4 = 0 , %d4は%d0(=sz)の一時保存レジスタ */                    
	movea.l %d2, %a0         /* %a0 = p */       

GETSTRING1_STEP1:
	cmp.l %d4, %d3           /* %d4 = size なら STEP2 へ */               
	beq GETSTRING1_STEP2
	moveq.l #2, %d0          /* %d0 = 2, キュー番号を2に */
	jsr OUTQ                 /* OUTQ(0,data) */      
	cmpi.l #0, %d0           /* 復帰値 %d0 が 0 なら STEP2 へ */           
	beq GETSTRING1_STEP2      
	move.b %d1, (%a0)+       /* %a0 番地へ data をコピー, %a0++ */                               
	addq.l #1, %d4           /* %d4++ */      
	bra GETSTRING1_STEP1      

GETSTRING1_STEP2:
	move.l %d4, %d0          /* %d0 = %d4 */ 
	
GETSTRING_END:
	movem.l (%sp)+, %d4/%a0
	rts
/*; ---- end include interget.s ----*/

/* ---- begin include timer.s ---- */
.section .bss
task_p:	.ds.b 4 /*タイマ割り込みで実行するプログラムの先頭アドレスを格納*/

/*タイマルーチン*/

.section .text

/*
タイマ1コントロールレジスタ
TCTL1 
    bit 4  : Interrupt Request Enable（比較割り込み） 0=disable, 1=enable
    bit 3-1: Clock Source 001 = 入力は SYSCLK．010 = 入力は SYSCLK/16．011 = 入力は TIN．1xx = 入力は CLK32．
    bit 0  : Timer Enable. 0=disable. 1=enable
（楠田・鴻上）
*/


/* RESET_TIMER
 *   タイマ割り込みを不可に。タイマを停止。（楠田・鴻上）
 */
RESET_TIMER:
	move.w #0x0004, TCTL1 | TCTL1=0b0_010_0
	rts

/* SET_TIMER:
 *   タイマ割り込み時に起動するサブルーチンを設定
 *   t * 0.1 msec 秒毎に割り込みが発生するように設定
 * 
 * %D1.W: タイマ割り込み発生周期 t
 * %D2.L: 割り込み時に起動するルーチンの先頭アドレス p
 * （楠田・鴻上）
 */
SET_TIMER:
	move.l %D2, task_p  /* 割り込み時に呼び出すサブルーチンのアドレスをtask_pにセット（楠田・鴻上） */
	move.w #206, TPRER1 /*カウンタ周波数を10000にする-> 周期0.1msec*/
	move.w %D1, TCMP1   /* TCMP1 = t （楠田・鴻上） */
	move.w #0x0015, TCTL1 /*比較割り込み許可, 1/16周期, タイマ許可*/
                          /* TCTL1 = 0b1_010_1 （楠田・鴻上）*/
	rts

/* CALL_RP:
 *   タイマ割り込み時に処理すべきルーチンを呼び出す
 * （楠田・鴻上）
 */
CALL_RP:
	movea.l task_p, %A0 /*task_pを使ってジャンプできないためA0レジスタにアドレスを入れてサブルーチンにジャンプさせる*/
	jsr  (%A0)  /* 割り込み時に起動するルーチンに遷移（楠田・鴻上） */
	rts
/*; ---- end include timer.s ----*/


uart1_interrupt:
/*送信割り込みベクタと受信割り込みベクタが同じであるため受信レジスタ、送信レジスタの値で送受信割り込みを区別する(室原)*/
    movem.l %D0-%D7/%A0-%A6, -(%SP) | 使用するレジスタをスタックに保存
    move.w UTX1, %D0                | UTX1をD0レジスタにコピーし保存しておく
    move.w %D0, %D1                 | 計算用にD1レジスタにコピー
    lsr.w #8, %D1
    lsr.w #7, %D1                   | 15回右シフト（上位ビットは0埋め）
    cmpi.w #1, %D1                  | 0=FIFOが空ではない, 1=空である（割り込み発生）
    bne UART1_INTR_SKIP_PUT         | 送信割り込みでないならスキップ 受信割り込みの処理に入る(室原)
    move.l #0, %D1                  | ch=%D1.L=0
    jsr INTERPUT
UART1_INTR_SKIP_PUT:
    move.w URX1, %D3                | 受信レジスタ URX1 を %D3.W にコピー
    move.b %D3, %D2                 | %D3.W の下位 8bit(データ部分) を %D2.B にコピー
    lsr.w #8, %D3
    lsr.w #5, %D3                   | 13回右シフト（上位ビットは0埋め）
    and.w #0x1, %D3                 | 0bit目以外を0に
    cmpi.w #1, %D3                  | 0 = 受信 FIFO にデータがない．1 = データがある
    bne UART1_INTR_SKIP_GET
    clr.l %D1                       | ch = %D1.L = 0, (data = %D2.Bは代入済)
    jsr INTERGET
UART1_INTR_SKIP_GET:
    movem.l (%SP)+, %D0-%D7/%A0-%A6 | レジスタを復帰
    rte
    
    
    
uart2_interrupt:
/*送信割り込みベクタと受信割り込みベクタが同じであるため受信レジスタ、送信レジスタの値で送受信割り込みを区別する(室原)*/
    movem.l %D0-%D7/%A0-%A6, -(%SP) | 使用するレジスタをスタックに保存
    move.w UTX2, %D0                | UTX2をD0レジスタにコピーし保存しておく
    move.w %D0, %D1                 | 計算用にD1レジスタにコピー
    lsr.w #8, %D1
    lsr.w #7, %D1                   | 15回右シフト（上位ビットは0埋め）
    cmpi.w #1, %D1                  | 0=FIFOが空ではない, 1=空である（割り込み発生）
    bne UART2_INTR_SKIP_PUT         | 送信割り込みでないならスキップ 受信割り込みの処理に入る(室原)
    move.l #1, %D1                  | ch=%D1.L=1
    jsr INTERPUT
UART2_INTR_SKIP_PUT:
    move.w URX2, %D3                | 受信レジスタ URX2 を %D3.W にコピー
    move.b %D3, %D2                 | %D3.W の下位 8bit(データ部分) を %D2.B にコピー
    lsr.w #8, %D3
    lsr.w #5, %D3                   | 13回右シフト（上位ビットは0埋め）
    and.w #0x1, %D3                 | 0bit目以外を0に
    cmpi.w #1, %D3                  | 0 = 受信 FIFO にデータがない．1 = データがある
    bne UART2_INTR_SKIP_GET
    move.l #1, %D1                  | ch = %D1.L = 1, (data = %D2.Bは代入済)
    jsr INTERGET
UART2_INTR_SKIP_GET:
    movem.l (%SP)+, %D0-%D7/%A0-%A6 | レジスタを復帰
    rte
    
    
    
tmr1_interrupt:
    movem.l %D0-%D7/%A0-%A6,-(%SP)  | 使用するレジスタをスタックに保存
    cmpi.l #5, %D0
    beq TMR1_TRUE
    move.w TSTAT1, %D0              | %D0=TSTAT1
    and.w #0x1, %D0                 | 0bit目以外を0に
    cmp.w #0, %D0
    beq TMR1_END                    | TSTAT1 の第 0 ビットが 1 となっているかどうかをチェックする．0 ならば rte で復帰 
    /*TASTAT1の第0ビットが0のときはコンペアイベントが起きていないためタイマ割り込みは起きてない, よって終了処理に移る->今回のタイマはコンペアイベントのみしか使わないため第0ビットだけ調べればよい(室原)*/
TMR1_TRUE:    
    clr.w TSTAT1                    | TSTAT1 を 0 クリア
    jsr CALL_RP /*タイマ割り込みが発生した時に処理すべきルーチンに移動するためにCALL_RPを呼び出す(室原)*/
TMR1_END:
    movem.l (%SP)+, %D0-%D7/%A0-%A6 | レジスタを復帰
    rte

.end
