.include "defs.s"
.global outbyte

.text
.even
outbyte:
    movem.l %D1-%D3/%A0, -(%SP)
outbyte_retry:
    move.l #SYSCALL_NUM_PUTSTRING, %D0
    movea.l %sp, %A0
    adda.l #20, %A0
    move.l (%A0), %D1
    adda.l #7, %A0
    move.l %A0, %D2
    move.l #1, %D3
    trap #0
    cmpi.l #0, %D0
    beq outbyte_retry
    movem.l (%SP)+, %D1-%D3/%A0
    rts
