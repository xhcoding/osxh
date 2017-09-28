#include "bootpack.h"

void wait_KBC_sendready(void) {
    // 等待键盘控制电路准备完毕
    for (;;) {
	if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
	    break;
	}
    }
}


void init_keyboard(void) {
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
}

struct FIFO8 keyfifo;

// 来自ps/2键盘的中断
void inthandler21(int *esp) {
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61); // 通知PIC IRQ-01受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo8_push(&keyfifo, data);
}

