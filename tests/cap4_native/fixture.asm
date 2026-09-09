; Authored test executable only. Never copied into or executed inside ETS2.
option casemap:none
PUBLIC cap_fixture,cap_end,cap_begin,cap_site,cap_upper,cap_continue,cap_fault
PUBLIC cap_state_invoke
EXTERN cap_registers:QWORD,cap_fx:BYTE,cap_fx_before:BYTE,cap_rsp_before:QWORD,cap_rsp_after:QWORD
.code
cap_fixture PROC FRAME
    push rbp
    .pushreg rbp
    sub rsp,180h
    .allocstack 180h
    lea rbp,[rsp+100h]
    .endprolog
    mov [rsp+68h],rcx
    mov [rsp+28h],rdx ; optional exception fixture callback, never instrumentation
    mov [rsp+30h],r8  ; optional fault injection pointer
    ALIGN 2
    nop ; same even-aligned two-byte site as the game
cap_begin LABEL NEAR
    cmp qword ptr [rsp+68h],4
    mov ecx,0FFFFFFFFh
    mov [rbp+40h],rsi
    mov byte ptr [rbp+68h],1
cap_site LABEL NEAR
    jae SHORT cap_upper
    mov qword ptr [rbp-78h],4
    jmp SHORT cap_continue
cap_upper LABEL NEAR
    cmp qword ptr [rsp+68h],10
    jbe SHORT cap_middle
    mov qword ptr [rbp-78h],10
    jmp SHORT cap_continue
cap_middle LABEL NEAR
    mov rax,[rsp+68h]
    mov [rbp-78h],rax
    test rax,rax
    je cap_zero
cap_continue LABEL NEAR
    ; The budget is the only consumed result of the authored clamp.
    mov rax,[rbp-78h]
    mov [rsp+20h],rax
    cmp qword ptr [rsp+30h],0
    je no_fault
    mov rax,[rsp+30h]
cap_fault LABEL NEAR
    mov r9d,[rax]
no_fault:
    cmp qword ptr [rsp+28h],0
    je no_callback
    mov rcx,[rsp+20h]
    call qword ptr [rsp+28h]
no_callback:
    mov rax,[rsp+20h]
    add rsp,180h
    pop rbp
    ret
    ; Force the original-shaped six-byte zero branch. Unreachable for admitted C.
    db 160 dup(90h)
cap_zero:
    xor eax,eax
    add rsp,180h
    pop rbp
    ret
cap_end LABEL NEAR
cap_fixture ENDP

; Compare all caller-visible registers except RAX (the changed budget result).
cap_state_invoke PROC FRAME
    push rbx
    .pushreg rbx
    push rbp
    .pushreg rbp
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    push r12
    .pushreg r12
    push r13
    .pushreg r13
    push r14
    .pushreg r14
    push r15
    .pushreg r15
    sub rsp,28h
    .allocstack 28h
    .endprolog
    mov rbx,103h
    mov rbp,105h
    mov rsi,106h
    mov rdi,107h
    mov r12,112h
    mov r13,113h
    mov r14,114h
    mov r15,115h
    xor edx,edx
    xor r8d,r8d
    mov r9,109h
    mov r10,110h
    mov r11,111h
    mov cap_rsp_before,rsp
    fxsave cap_fx_before
    call cap_fixture
    mov cap_rsp_after,rsp
    mov cap_registers,rax
    mov cap_registers+8,rcx
    mov cap_registers+16,rdx
    mov cap_registers+24,r8
    mov cap_registers+32,r9
    mov cap_registers+40,r10
    mov cap_registers+48,r11
    mov cap_registers+56,rbx
    mov cap_registers+64,rbp
    mov cap_registers+72,rsi
    mov cap_registers+80,rdi
    mov cap_registers+88,r12
    mov cap_registers+96,r13
    mov cap_registers+104,r14
    mov cap_registers+112,r15
    pushfq
    pop cap_registers+120
    fxsave cap_fx
    add rsp,28h
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret
cap_state_invoke ENDP
END
