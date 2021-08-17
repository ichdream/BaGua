[bits 32]
section .text
global switch_to
; --------------------------------------------
; switch_to接受两个参数，第一个参数是当前线程cur，第二个参数是下一个将要运行的线程next
; 函数的功能：保存cur线程的寄存器映像，将下一个线程next的寄存器映像载入处理器中；
; 程序13～16行是遵循ABI原则
;
; --------------------------------------------
switch_to:
    ; 将中断处理函数的上下文保存到目标PCB的栈中
    ; 这里隐含的是下次调度返回的地址其实是switch_to的返回地址
    push esi
    push edi
    push ebx
    push ebp
    mov eax, [esp + 20]             ; 得到栈中的参数cur, cur = [esp + 20]
    mov [eax], esp                  ; 保存栈指针esp到task_struct的self_kstack字段

    ; 跳到目标PCB执行
    mov eax, [esp + 24]
    mov esp, [eax]
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret