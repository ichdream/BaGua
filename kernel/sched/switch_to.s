; -------------------------------------------------
; 	线程切换的实现
;
; -------------------------------------------------

[global switch_to]

; 具体的线程切换操作，重点在于寄存器的保存与恢复
; 函数原型：switch_to(&(prev->context), &(current->context));
; 函数参数入栈时，先入current->context的地址
; mov指令格式：mov dest org， det: 目标地址， org原地址 
switch_to:
        mov eax, [esp+4]      ; 将[esp+4]的值（地址）存入eax寄存器中， 即prev的地址

        mov [eax+0],  esp     ; 将esp寄存器的值存入地址[eax+0]处
        mov [eax+4],  ebp     ; 将ebp寄存器的值存入地址[eax+0]处
        mov [eax+8],  ebx
        mov [eax+12], esi
        mov [eax+16], edi
        pushf
        pop ecx
        mov [eax+20], ecx

        mov eax, [esp+8]

        mov esp, [eax+0]
        mov ebp, [eax+4]
        mov ebx, [eax+8]
        mov esi, [eax+12]
        mov edi, [eax+16]
        mov eax, [eax+20]
        push eax
        popf
 	
        ret
