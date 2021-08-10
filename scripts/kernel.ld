/*
* 系统编译链接时使用的链接器脚本文件
*   1）声明内核程序的入口符号地址：start
    2）指导链接器构造相应的内核镜像文件
        a) .text段，以及其起始地址为0x100000(1M)
        b) .data段，4kb对齐
        c) .bss段，4kb对齐
        d）.rodata，只读的已初始化数据段
*/

ENTRY(start)
SECTIONS
{
    . = 0x100000;

    .text :
    {
        *(.text)
        . = ALIGN(4096);
    }
    .data :
    {
        *(.data)
        *(.rodata)
        . = ALIGN(4096);
    }
    .bss :
    {
        *(.bss)
        . = ALIGN(4096);
    }
    .stab :
    {
        *(.stab)
        . = ALIGN(4096);
    }
    .stabstr :
    {
        *(.stabstr)
        . = ALIGN(4096);
    }

    /DISCARD/ : { *(.comment) *(.eh_frame) }
}