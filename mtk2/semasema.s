.section .text

*****************************
	***pv_handler 40ページ！
	***D0の値でPかVか半別する
	***D1を引数としてp_body/v_bodyに渡す
*****************************
.global pv_handler
.even
pv_handler:
	movem.l %D0-%D1, -(%SP) /*スーパーバイザモードなうだからssp、ユーザ側ならsp。今回はtrap #1が使われるのでスーパーバイザモード*/
	move.w %SR, -(%SP)      /* SRの値をスタックに退避 */
	move.w #0x2700, %SR     /* 走行レベルを7にする。特権モード2700=2進数で0010 0111 0000 0000！*/
	move.l %D1, -(%SP)      /*D1 を引数としてスタックに積む(p/v)*/
	cmpi.l #2, %D0          /*D0=0 ->P操作それ以外　D0≠0 ->V*/
	beq waitpbra
	cmpi.l #1, %D0
	beq vbra
pbra:
	jsr p_body /*P操作本体飛び*/
	bra finish
vbra:
	jsr v_body
	bra finish
waitpbra:
	jsr waitp_body

finish:
    addq.l #4, %SP     /* 引数分のスタックを戻す */
	move.w (%SP)+, %SR /* SRを復帰（走行レベルを戻す） */
	movem.l (%SP)+, %D0-%D1
	rte

*****************************
***P
*****************************
.global P
.even
P:
    link.w %FP, #0
    movem.l %D0-%D1, -(%SP)
    move.l #0, %D0     /* PシステムコールID */
    move.l 8(%FP), %D1 /* 引数 */
	trap #1            /*スーパーバイザモードに入って、ベクタテーブル？のtrap#1のエントリに飛ぶ*/
    movem.l (%SP)+, %D0-%D1
    unlk %FP
	rts

*****************************
***V
*****************************
.global V
.even
V:
    link.w %FP, #0
    movem.l %D0-%D1, -(%SP)
    move.l #1, %D0     /* VシステムコールID */
    move.l 8(%FP), %D1 /* 引数 */
	trap #1            /*スーパーバイザモードに入って、ベクタテーブル？のtrap#1のエントリに飛ぶ*/
    movem.l (%SP)+, %D0-%D1
    unlk %FP
    rts
*****************************
.global waitP
.even
waitP:
    link.w %FP, #0
    movem.l %D0-%D1, -(%SP)
    move.l #2, %D0     /* PシステムコールID */
    move.l 8(%FP), %D1 /* 引数 */
    trap #1            /*スーパーバイザモードに入って、ベクタテーブル？のtrap#1のエントリに飛ぶ
*/
    movem.l (%SP)+, %D0-%D1
    unlk %FP
    rts


