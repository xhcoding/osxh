#include <stdio.h>
#include "bootpack.h"

void init_pic(void) {
    io_out8(PIC0_IMR, 0xff); // 禁止所有中断
    io_out8(PIC1_IMR, 0xff); // 禁止所有中断

    io_out8(PIC0_ICW1, 0x11); // 边沿触发模式
    io_out8(PIC0_ICW2, 0x20); // IRQ0-7由INT20-27接收
    io_out8(PIC0_ICW3, 1 << 2); // PIC1由IRQ2连接
    io_out8(PIC0_ICW4, 0x01); // 无缓冲区模式

    io_out8(PIC1_ICW1, 0x11); // 边沿触发模式
    io_out8(PIC1_ICW2, 0x28); // IRQ8-15由INT28-2f接收
    io_out8(PIC1_ICW3, 2); // PIC1由IRQ2连接
    io_out8(PIC1_ICW4, 0x01); // 无缓冲区模式

    io_out8(PIC0_IMR, 0xfb); // 11111011 PCI1以外全部禁止
    io_out8(PIC1_IMR, 0xff); // 禁止所有中断
    
}


struct FIFO8 keyfifo;

// 来自ps/2键盘的中断
void inthandler21(int *esp) {
    unsigned char data;
    io_out8(PIC0_OCW2, 0x61); // 通知PIC IRQ-01受理完毕
    data = io_in8(PORT_KEYDAT);
    fifo8_push(&keyfifo, data);
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

void inthandler27(int *esp) {
    struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
    boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
    putfonts8_asc(binfo->vram, binfo->scrnx, 50, 50, COL8_FFFFFF, "27");

    for (;;) {
	io_hlt();
    }
}
