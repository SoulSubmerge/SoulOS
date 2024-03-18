[bits 32]

section .text

global taskSwitch
taskSwitch:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov eax, esp;
    and eax, 0xfffff000; current

    mov [eax], esp

    mov eax, [ebp + 8]; next
    mov esp, [eax]

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret

global getRunningTaskEsp
getRunningTaskEsp:
    mov eax, esp
    add eax, 0x08
    and eax, 0xfffff000
    ret