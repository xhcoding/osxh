#include "bootpack.h"

void enable_mouse(struct MOUSE_DEC *mdec) {
    // 激活鼠标
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    mdec->phase = 0;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat) {
    if (mdec->phase == 0) {
	if (dat == 0xfa) {
	    mdec->phase = 1;
	}
	return 0;
    }
    if (mdec->phase == 1) {
	if ((dat & 0xc8) == 0x08) {
	    mdec->buf[0] = dat;
	    mdec->phase = 2;
	}
	return 0;
    }
    if (mdec->phase == 2) {
	mdec->buf[1] = dat;
	mdec->phase = 3;
	return 0;
    }
    if (mdec->phase == 3) {
	mdec->buf[2] = dat;
	mdec->phase = 1;
	mdec->btn = mdec->buf[0] & 0x07;
	mdec->x = mdec->buf[1];
	mdec->y = mdec->buf[2];
	if ((mdec->buf[0] & 0x10) != 0) {
	    mdec->x |= 0xffffff00;
	}
	if ((mdec->buf[0] & 0x20) != 0) {
	    mdec->y |= 0xffffff00; 
	}
	mdec->y = -mdec->y;
	return 1;
    }
    return -1;
}


struct FIFO8 mousefifo;

// 来自鼠标的中断
void inthandler2c(int *esp) {
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64); // 通知PIC1，IRQ-12受理完成
    io_out8(PIC0_OCW2, 0x62); // 通知PIC0， IRC-02受理完成
    data = io_in8(PORT_KEYDAT);
    fifo8_push(&mousefifo, data);
}

