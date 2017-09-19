;; hello-os
;; tab 
;; 以下这段代码是标准的FAT32格式软盘专用代码

	DB 0xeb, 0x4e, 0x90
	DB "HELLOIPL"		; 启动区的名称，可以是任意字符(8字节)
	DW 512			; 每个扇区的大小，必须为512字节
	DB 1			; 簇(cluster)的大小，必须为一个扇区
	
	
	
