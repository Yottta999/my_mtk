.include "defs.s"
.global inbyte

.text
.even
inbyte:
    link.w %FP, #-4
    movem.l %D1-%D3/%A0, -(%SP)
inbyte_retry:
    move.l #SYSCALL_NUM_GETSTRING, %D0
    movea.l %sp, %A0
    adda.l #28, %A0
    move.l (%A0), %D1
    move.l %FP, %D2
    subi.l #4, %D2
    move.l #1, %D3
    trap #0
    cmpi.l #1, %D0
    bne inbyte_retry
    clr.l %D0
    move.b -4(%FP), %D0
    movem.l (%SP)+, %D1-%D3/%A0
    unlk %FP
    rts
