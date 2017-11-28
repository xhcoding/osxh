#include "bootpack.h"

void wait_KBC_sendready(void) {
    // 等待键盘控制电路准备完毕
    for (;;) {
	if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
	    break;
	}
    }
}


struct FIFO32* keyfifo;
int keydata0;

void init_keyboard(struct FIFO32 *fifo, int data0) {
    // 将FIFO缓冲区的信息保存在全局变量里
    keyfifo = fifo;
    keydata0 = data0;

    // 键盘控制器的初始化
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
}


// 来自ps/2键盘的中断
void inthandler21(int *esp) {
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61); // 通知PIC IRQ-01受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo32_push(keyfifo, data + keydata0);
}

