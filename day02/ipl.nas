;; hello-os
;; tab 
;; 以下这段代码是标准的FAT32格式软盘专用代码

ORG 0x7c00		        ; 指明程序的装载地址

start:
	JMP entry
	DB 0xeb, 0x4e, 0x90
	DB "HELLOIPL"		; 启动区的名称，可以是任意字符(8字节)
	DW 512			; 每个扇区的大小，必须为512字节
	DB 1			; 簇(cluster)的大小，必须为一个扇区
	DW 1			; FAT的起始位置(一般从第一个地方开始)	
	DB 2			; FAT的个数，必须为2
	DW 224			; 根目录的大小，一般设成224
	DW 2880			; 该磁盘的大小， 必须是2880
	DB 0xf0			; 磁盘的种类
	DW 9			; FAT的长度
	DW 18			; 一个磁道有几个扇区(必须是18)
	DW 2			; 磁头数，必须为2
	DD 0			; 必须是0
	DD 2880			; 重写一次磁盘大小
	DB 0,0,0x29		; 意义不明，固定
	DD 0xffffffff		; 可能是卷标号码
	DB "HELLO-OS   "   	; 磁盘名称,11字节
	DB "FAT32   "		; 磁盘格式名称,8字节
	RESB 18			; 空出18字节

;; 程序主体
entry:
	MOV AX, 0
	MOV SS, AX
	MOV SP, 0x7c00
	MOV DS, AX
	MOV ES, AX

	MOV SI, msg

putloop:
	MOV AL, [SI]
	ADD SI, 1
	CMP AL, 0
	JE fin
	MOV AH, 0x0e		; 显示一个文字
	MOV BX, 15		; 指定字符颜色
	INT 0X10		; 调用显卡BIOS
	JMP putloop

fin:
	HLT			; 让cpu停止，等待指令
	JMP fin			; 无限循环
;; 信息显示
msg:
	DB 0x0a, 0x0a		; 两个换行
	DB "Hello, xhcoding"
	DB 0x0a
	DB 0
market:
	RESB 0x1fe-(market - start)		; 填写0x00直到0x00fe
	DB 0x55, 0xaa


