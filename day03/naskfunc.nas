;; naskfunc
[FORMAT "WCOFF"]	        ; 制作目标文件的模式
[BITS 32]	                ; 使用32位模式

;; 制作目标文件信息
[FILE "naskfunc.nas"]		; 源文件名信息
GLOBAL _io_hlt		        ; 程序中包含的函数名

[SECTION .text]			; 目标文件写了这些后再写程序
_io_hlt:			; void io_hlt(void)
	HLT
	RET
	
