[INSTRSET "i486p"]
;;vbe
	VBEMODE EQU 0x105
;; xh-os

	BOTPAK  EQU 0x00280000	; 加载bootpack
	DSKCAC  EQU 0x00100000	; 磁盘缓存位置
	DSKCAC0 EQU 0x00008000	; 磁盘缓存的位置(实模式)
;; boot info
	CYLS    EQU 0X0ff0	; 设定启动区
	LEDS    EQU 0x0ff1	;
	VMMODE  EQU 0x0ff2	; 颜色信息
	SCRNX  EQU 0x0ff4	; 分辨率x
	SCRNY  EQU 0x0ff6	; 分辨率y
	VRAM    EQU 0x0ff8	; 图像缓冲区的开始地址
	
ORG 0xc200			; 程序在内存中的位置

;; 确认vbe是否存在
	MOV AX, 0x9000
	MOV ES, AX
	MOV DI, 0
	MOV AX, 0x4f00
	INT 0x10
	CMP AX, 0x004f
	JNE scrn320

;; 检查VBE版本
	MOV AX, [ES:DI + 4]
	CMP AX, 0x0200
	JB scrn320

;; 取得画面信息模式
	MOV CX, VBEMODE
	MOV AX, 0x4f01
	INT 0x10
	CMP AX, 0x004f
	JNE scrn320

;; 画面模式信息的确认
	CMP BYTE [ES:DI + 0x19], 8
	JNE scrn320
	CMP BYTE [ES:DI + 0x1b], 4
	JNE scrn320
	MOV AX, [ES:DI + 0x00]
	AND AX, 0x0080
	JZ scrn320

;; 画面模式的切换
	MOV BX, VBEMODE + 0x4000
	MOV AX, 0x4f02
	INT 0x10
	MOV BYTE [VMMODE], 8
	MOV AX, [ES:DI + 0x12]
	MOV [SCRNX], AX
	MOV AX, [ES:DI + 0x14]
	MOV [SCRNY], AX
	MOV EAX, [ES:DI + 0x28]
	MOV [VRAM], EAX
	JMP keystatus
	

scrn320:
	MOV AL, 0x13 		; VGA, 320 * 200 * 8
	MOV AH, 0x00
	INT 0x10
	MOV BYTE [VMMODE], 8
	MOV WORD [SCRNX], 320
	MOV WORD [SCRNY], 200
	MOV DWORD [VRAM], 0X000a000

keystatus:
	
;; 用bios取得键盘上的各种LED灯的状态
	MOV AH, 0x02
	int 0x16		; 键盘bios
	MOV [LEDS], AL

;; 防止PIC接受所有中断
	MOV AL, 0xff
	OUT 0x21, AL
	NOP
	OUT 0xa1, AL
	CLI

;; 让cpu支持1M以上内存
	CALL waitbdout
	MOV AL, 0xd1
	OUT 0x64, AL
	CALL waitbdout
	MOV AL, 0xdf
	OUT 0x60, AL
	CALL waitbdout

;; 保护模式转换
[INSTRSET "i486p"]		; 说明使用486指令
	LGDT [GDTR0]		; 设置临时GDT
	MOV EAX, CR0
	AND EAX, 0x7fffffff	; 使用bit31,禁用分页
	OR EAX, 0x00000001	; bit0到1转换，保护模式过渡
	MOV CR0, EAX		;
	JMP pipelineflush

pipelineflush:
	MOV AX, 1*8		; 写32bit的段
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX
	MOV SS, AX

;; backpack传递
	MOV ESI, bootpack	; 源
	MOV EDI, BOTPAK		; 目标
	MOV ECX, 512 * 1024 / 4
	CALL memcpy

;; 传输磁盘数据
;; 从引导区开始
	MOV ESI, 0x7c00		; 源
	MOV EDI, DSKCAC		; 目标
	MOV ECX, 512 / 4
	call memcpy

;; 剩余的全部
	MOV ESI, DSKCAC0+512 	; 源
	MOV EDI, DSKCAC+512	; 目标
	MOV ECX, 0
	MOV CL, BYTE [CYLS]
	IMUL ECX, 512 * 18 * 2 / 4
	SUB ECX, 512 / 4	; IPL偏移量
	CALL memcpy

;; bootpack启动
	MOV EBX, BOTPAK
	MOV ECX, [EBX+16]
	ADD ECX, 3
	SHR ECX, 2
	JZ skip
	MOV ESI, [EBX+20]
	ADD ESI, EBX
	MOV EDI, [EBX+12]
	CALL memcpy

skip:
	MOV ESP, [EBX+12]	; 堆栈初始化
	JMP DWORD 2*8:0x0000001b

waitbdout:
	IN AL, 0x64
	AND AL, 0x02
	JNZ waitbdout
	RET

memcpy:
	MOV EAX, [ESI]
	ADD ESI, 4
	MOV [EDI], EAX
	ADD EDI, 4
	SUB ECX, 1
	JNZ memcpy
	RET

;;memcpy地址前缀大小
ALIGNB 16
GDT0:
	RESB 8			; 初始值
	DW 0xffff, 0x0000, 0x9200, 0x00cf ; 写32bit位段寄存器
	DW 0xffff, 0x0000, 0x9a28, 0x0047 ; 可执行文件的32寄存器，bootpack用
	DW 0
GDTR0:
	DW 8*3 - 1
	DD GDT0

ALIGNB 16

bootpack:
