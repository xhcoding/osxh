;; xh-os

;; boot info
	CYLS   EQU 0X0ff0	; 设定启动区
	LEDS   EQU 0x0ff1	;
	VMMODE EQU 0x0ff2	; 颜色信息
	SCRENX EQU 0x0ff4	; 分辨率x
	SCRENY EQU 0x0ff6	; 分辨率y
	VRAM   EQU 0x0ff8	; 图像缓冲区的开始地址
	
ORG 0xc200			; 程序在内存中的位置
	
	MOV AL, 0x13		; VGA显卡，300 * 200 * 8位彩色
	MOV AH, 0x00		;
	INT 0x10
	MOV BYTE [VMMODE], 8	;记录画面模式
	MOV WORD [SCRENX], 320
	MOV WORD [SCRENY], 200
	MOV DWORD [VRAM], 0x000a0000

;; 用bios取得键盘上的各种LED灯的状态
	MOV AH, 0x02
	int 0x16		; 键盘bios
	MOV [LEDS], AL

fin:
	HLT
	JMP fin
