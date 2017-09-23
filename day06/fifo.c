#include "bootpack.h"


void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size; // 缓冲区大小
    fifo->flags = 0;
    fifo->p = 0; // 栈顶
    fifo->q = 0; // 栈顶
}


int fifo8_push(struct FIFO8 *fifo, unsigned char data) {
    if (fifo->free == 0) {
	fifo->flags |= FLAGS_OVERRUN; // 栈溢出
	return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size) {
	fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

int fifo8_pop(struct FIFO8 *fifo) {
    int data;
    if (fifo->free == fifo->size) {
	// 缓冲区为空
	return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size) {
	fifo->q = 0;
    }
    fifo->free++;
    return data;
}

int fifo8_status(struct FIFO8 *fifo) {
    return fifo->size - fifo->free;
}
