/* レジスタ定義 */
.equ REGBASE, 0xfff000 | DMAP を使用．
.equ IOBASE, 0x00d00000

/* 割り込み関係のレジスタ*/
.equ IVR, REGBASE+0x300 | 割り込みベクタレジスタ
.equ IMR, REGBASE+0x304 | 割り込みマスクレジスタ
.equ ISR, REGBASE+0x30c | 割り込みステータスレジスタ
.equ IPR, REGBASE+0x310 | 割り込みペンディングレジスタ

/* タイマ関係のレジスタ */
.equ TCTL1, REGBASE+0x600  | タイマ１コントロールレジスタ
.equ TPRER1, REGBASE+0x602 | タイマ１プリスケーラレジスタ
.equ TCMP1, REGBASE+0x604  | タイマ１コンペアレジスタ
.equ TCN1, REGBASE+0x608   | タイマ１カウンタレジスタ
.equ TSTAT1, REGBASE+0x60a | タイマ１ステータスレジスタ

/* UART1（送受信）関係のレジスタ */
.equ USTCNT1, REGBASE+0x900 | UART1 ステータス/コントロールレジスタ
.equ UBAUD1, REGBASE+0x902  | UART1 ボーコントロールレジスタ
.equ URX1, REGBASE+0x904    | UART1 受信レジスタ
.equ UTX1, REGBASE+0x906    | UART1 送信レジスタ

/* UART2（送受信）関係のレジスタ */
.equ USTCNT2, REGBASE+0x910 | UART2 ステータス/コントロールレジスタ
.equ UBAUD2, REGBASE+0x912  | UART2 ボーコントロールレジスタ
.equ URX2, REGBASE+0x914    | UART2 受信レジスタ
.equ UTX2, REGBASE+0x916    | UART2 送信レジスタ

/* LED */
/*ここで指定されたアドレスに表示させたい文字コードを入れることでLED表示を変更できる(室原)*/
.equ LED7, IOBASE+0x000002f | ボード搭載の LED 用レジスタ
.equ LED6, IOBASE+0x000002d 
.equ LED5, IOBASE+0x000002b
.equ LED4, IOBASE+0x0000029
.equ LED3, IOBASE+0x000003f
.equ LED2, IOBASE+0x000003d
.equ LED1, IOBASE+0x000003b
.equ LED0, IOBASE+0x0000039

/*D0の値で呼び出すサブルーチンを決めている。ここでは何を呼び出しているかを分かりやすくするためにシンボルに数値を定義している(室原)*/
.equ SYSCALL_NUM_GETSTRING,   1 |文字列入力（GETSTRING）（ここからコメント鴻上）|
.equ SYSCALL_NUM_PUTSTRING,   2 |文字列出力(PUTSTRING)|
.equ SYSCALL_NUM_RESET_TIMER, 3 |タイマリセット|
.equ SYSCALL_NUM_SET_TIMER,   4 |タイマセット|
.equ SYSCALL_NUM_SKIPMT, 5
