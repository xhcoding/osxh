#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(void) {
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next = 0xffffffff;
    for (i = 0; i < MAX_TIMER; i++) {
	timerctl.timers0[i].flags = 0; // 未使用
    }
}

struct TIMER *timer_alloc(void) {
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
	if (timerctl.timers0[i].flags == 0) {
	    timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
	    return &timerctl.timers0[i];
	}
    }
    return 0;
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0; // 未使用
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data) {
    timer->fifo = fifo;
    timer->data = data;
}

void inthandler20(int *esp) {
    io_out8(PIC0_OCW2, 0x60); // IRQ-00信号接受完了的信息通知给PIC
    timerctl.count++;

    if (timerctl.next > timerctl.count) {
	return;
    }
    timerctl.next = 0xffffffff;
    
    int i, j;
    for (i = 0; i < timerctl.use; i++) {
	// timers的定时器都处于动作中，所以不确认flags
	if (timerctl.timers[i]->timeout > timerctl.count) {
	    break;
	}
	// 超时
	timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
	fifo8_push(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    // 正好有i个计时器超时了，其他的移位
    timerctl.use -= i;
    for (j = 0; j < timerctl.use; j++) {
	timerctl.timers[j] = timerctl.timers[i + j];
    }
    if (timerctl.use > 0) {
	timerctl.next = timerctl.timers[0]->timeout;
    } else {
	timerctl.next = 0xffffffff;
    }
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    int e, i, j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    io_cli();
    // 搜索注册位置
    for (i = 0; i > timerctl.use; i++) {
	if (timerctl.timers[i]->timeout >= timer->timeout) {
	    break;
	}
    }
    // i之后全部后移
    for (j = timerctl.use; j > i; j--) {
	timerctl.timers[j] = timerctl.timers[j - 1];
    }
    
    timerctl.use++;
    // 插在空位上
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e);
}

