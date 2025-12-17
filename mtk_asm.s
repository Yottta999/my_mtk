.include "defs.s"
.section .text

.equ SIZEOF_TCB_TYPE, 20
.equ TCB_TYPE_STACK_PTR_OFFSET, 4
.equ TCB_TYPE_NEXT_OFFSET, 16

.global first_task
.even
first_task:
    * 1. TCB 先頭番地の計算：curr_task の TCB のアドレスを見つける
    move.l curr_task, %D1      | %D1.l = curr_task
    muls #SIZEOF_TCB_TYPE, %D1 | %D1.l = curr_task * SIZEOF_TCB_TYPE
    lea.l task_tab, %A1        | %A1.l = task_tab
    add.l %D1, %A1             | %A1.l = &task_tab[curr_task]

    move.l %sp, SINGLE_SSP

    * 2. USP，SSP の値の回復
    * %SSP = &task_tab[curr_task]->stack_ptr
    move.l TCB_TYPE_STACK_PTR_OFFSET(%A1), %SP
    move.l (%SP)+, %A1
    move.l %A1, %USP

    * 3. 残りの全レジスタの回復
    movem.l (%SP)+, %D0-%D7/%A0-%A6

    * 4. ユーザタスクの起動（SR,PCの復帰）
    rte
BACKTO_SINGLE:
    rts
.data
.even
SINGLE_SSP: .ds.l 1

.section .text
.global swtch
.even
swtch:
    * 1. SR をスタックに積んで，RTE で復帰できるようにする．
    move.w %SR, -(%SP) 

    * 2. 実行中のタスクのレジスタの退避
    movem.l %D0-%D7/%A0-%A6, -(%SP)
    move.l %USP, %A1
    move.l %A1, -(%SP)

    * 3. SSPの保存
    move.l curr_task, %D1      | %D1.l = curr_task
    muls #SIZEOF_TCB_TYPE, %D1 | %D1.l = curr_task * SIZEOF_TCB_TYPE
    lea.l task_tab, %A1        | %A1.l = task_tab
    add.l %D1, %A1             | %A1.l = &task_tab[curr_task]
    move.l %SP, TCB_TYPE_STACK_PTR_OFFSET(%A1)

    * 4. curr_task を変更
    move.l next_task, curr_task

    * 5. 次のタスクの SSP の読み出し
    move.l curr_task, %D1      | %D1.l = curr_task
    muls #SIZEOF_TCB_TYPE, %D1 | %D1.l = curr_task * SIZEOF_TCB_TYPE
    lea.l task_tab, %A1        | %A1.l = task_tab
    add.l %D1, %A1             | %A1.l = &task_tab[curr_task]
    * %SP(SSP) = &task_tab[curr_task]->stack_ptr
    move.l TCB_TYPE_STACK_PTR_OFFSET(%A1), %SP

    * 6. 次のタスクのレジスタの読み出し
    move.l (%SP)+, %A1
    move.l %A1, %USP
    movem.l (%SP)+, %D0-%D7/%A0-%A6

    * 7. タスク切り替え 
    rte

* タイマ関連のサブルーチン
.even
hard_clock:
    movem.l %D1/%A1, -(%SP)

    cmpi.l #0, ready
    bne exe_addq
    move.l curr_task, ready
    * TCB 先頭番地の計算：ready の TCB のアドレスを見つける
    move.l ready, %D1          | %D1.l = ready
    muls #SIZEOF_TCB_TYPE, %D1 | %D1.l = ready * SIZEOF_TCB_TYPE
    lea.l task_tab, %A1        | %A1.l = task_tab
    add.l %D1, %A1             | %A1.l = &task_tab[ready]
    move.l #0, TCB_TYPE_NEXT_OFFSET(%A1)
    bra end_addq
exe_addq:
    * addqに渡す引数をスタックに詰める（右から左）
    move.l curr_task, -(%SP)
    * TCB 先頭番地の計算：ready の TCB のアドレスを見つける
    move.l ready, %D1          | %D1.l = ready
    muls #SIZEOF_TCB_TYPE, %D1 | %D1.l = ready * SIZEOF_TCB_TYPE
    lea.l task_tab, %A1        | %A1.l = task_tab
    add.l %D1, %A1             | %A1.l = &task_tab[ready]
    move.l %A1, -(%SP)
	jsr addq /*addqの呼び出し*/
    addq.l #8, %SP /* %SPを戻す */
end_addq:
	jsr sched /*schedの呼びだし*/
	jsr swtch /*swtchの呼び出し*/
    movem.l (%SP)+, %D1/%A1
	rts

.global init_timer
.even
init_timer:
	/*タイマのリセットをする*/
	move.l #SYSCALL_NUM_RESET_TIMER, %D0
	trap #0
	/*タイマのセットをする*/
	move.l #SYSCALL_NUM_SET_TIMER, %D0
	move.w #10000, %D1 /*1秒に設定*/
	move.l #hard_clock, %D2 /*hard_clockを呼び出すよう設定*/
	trap #0
        rts

.global skipmt
.even
skipmt:
        move.l #SYSCALL_NUM_SKIPMT, %D0
        trap #0
        rts

.include "semasema.s"

.section .bss
.extern task_tab
.extern curr_task
.extern next_task
