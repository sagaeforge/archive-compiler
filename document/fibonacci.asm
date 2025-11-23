section .data

section .text
global _start


fibonacci: ; fibonacci
    push rbp
    mov rbp, rsp
    mov [rbp-8], rdi
    sub rsp, 64
    mov rax, [rbp-8]
    mov rcx, 2
    cmp rax, rcx
    setl al
    movzx rax, al
    cmp rax, 0
    je endif1
    mov rdx, [rbp-8]
    mov rax, rdx
    mov rsp, rbp
    pop rbp
    ret
endif1: ; endif1
    mov r8, [rbp-8]
    mov r9, 1
    sub r8, r9
    mov rdi, r8
    call fibonacci
    push rax
    mov r10, [rbp-8]
    mov r11, 2
    sub r10, r11
    mov rdi, r10
    call fibonacci
    pop rcx
    add rcx, rax
    mov rax, rcx
    mov rsp, rbp
    pop rbp
    ret
    mov rsp, rbp
    pop rbp
    ret

_start: ; _start
    mov rbx, 5
    mov rdi, rbx
    call fibonacci
    mov rdi, rax
    mov rax, 33554433
    syscall