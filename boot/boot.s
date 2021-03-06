; -----------------------------------------------
;   multiBoot.s 内核加载启动位置
;   遵循Gnu grub multiboot/multiboot2 多系统引导协议
;   ref: https://www.gnu.org/software/grub/manual/multiboot/multiboot.html

; 符合Multiboot规范的 OS 映象需要这样一个 magic Multiboot 头

    ; Multiboot 头的分布必须如下表所示：
    ; ----------------------------------------------------------
    ; 偏移量  类型  域名        备注
    ;
    ;   0     u32   magic       必需
    ;   4     u32   flags       必需
    ;   8     u32   checksum    必需
    ;
; -----------------------------------------------

; multiboot引导协议, 兼容版本一和二
MBOOT_HEADER_MAGIC  equ     0x1BADB002  ; Multiboot 魔数，由规范决定的
MBOOT2_HEADER_MAGIC  equ     0x1BADB002  ; Multiboot2 魔数，由规范决定的

MBOOT_PAGE_ALIGN    equ     1 << 0      ; 0 号位表示所有的引导模块将按页(4KB)边界对齐
MBOOT_MEM_INFO      equ     1 << 1      ; 1 号位通过 Multiboot 信息结构的 mem_* 域包括可用内存的信息
                                        ; (告诉GRUB把内存空间的信息包含在Multiboot信息结构中)
; Multiboot 的标志
MBOOT_HEADER_FLAGS  equ     MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO

; 域checksum是一个32位的无符号值，当与其他的magic域(也就是magic和flags)相加时，
; 要求其结果必须是32位的无符号值 0 (即magic + flags + checksum = 0)
MBOOT_CHECKSUM      equ     - (MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)
MBOOT2_CHECKSUM     equ     - (MBOOT2_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

; -----------------------------------------------
[BITS 32]
section .init.text
; ---------- GRUB协议header定义-------------
_start:
    jmp _entry
    ALIGN 8
mbt_hdr:
    dd MBOOT_HEADER_MAGIC
    dd MBOOT_HEADER_FLAGS
    dd -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)
    dd mbt_hdr
    dd _start
    dd 0
    dd 0
    dd _entry
; --------- GRUB2协议header定义-------------------
    ALIGN 8
mbt2_hdr:
    dd MBOOT2_HEADER_MAGIC
    dd 0
    dd mbt2_hdr_end - mbt2_hdr
    dd -(MBOOT2_HEADER_MAGIC + 0 + (mbt2_hdr_end - mbt2_hdr))
    dw 2, 0
    dd 24
    dd mbt2_hdr
    dd _start
    dd 0
    dd 0
    dw 3, 0
    dd 12
    dd _entry
    dd 0
    dw 0, 0
    dd 8
mbt2_hdr_end:
    ALIGN 8


[GLOBAL _start]      ; 内核代码入口，此处提供该声明给 ld 链接器
[GLOBAL mboot_ptr_tmp]  ; 全局的 struct multiboot * 变量
[EXTERN kern_entry]     ; 声明内核 C 代码的入口函数

_entry:
    cli
    mov [mboot_ptr_tmp], ebx ; 在ebx中存放mboot_ptr_tmp所指的值
    mov esp, STACK_TOP       ; 设置内核栈地址
    and esp, 0FFFFFFF0H  ; 栈地址按照16字节对齐
    mov ebp, 0       ; 帧指针修改为 0
    call kern_entry      ; 调用内核入口函数

; ---------------------------------------------------------------

section .init.data             ; 未初始化的数据段从这里开始
stack:  times 1024 db 0       ; 这里作为内核栈

STACK_TOP equ $-stack-1      ; 内核栈顶，$ 符指代是当前地址

mboot_ptr_tmp: dd 0        ; 全局的multiboot 结构体指针