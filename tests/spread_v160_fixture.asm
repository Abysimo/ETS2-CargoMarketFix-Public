OPTION CASEMAP:NONE
EXTERN s160_visit:PROC
EXTERN cmf_refresh_spread_bridge_v160:PROC
EXTERN s160_input:QWORD
EXTERN s160_regs:QWORD
EXTERN s160_fx_before:BYTE
EXTERN s160_fx_after:BYTE
EXTERN s160_rsp_before:QWORD
EXTERN s160_rsp_after:QWORD
PUBLIC s160_fixture
PUBLIC s160_site
PUBLIC s160_loop
PUBLIC s160_finish
PUBLIC s160_end
PUBLIC s160_probe
.code
s160_fixture PROC FRAME
    push rbx
    .pushreg rbx
    push rbp
    .pushreg rbp
    push rdi
    .pushreg rdi
    sub rsp,20h
    .allocstack 20h
    .endprolog
    mov rax,rcx
    mov rcx,rdx
    mov rbp,r8
    mov rbx,rax
    lea rdi,[rax+rcx*8]
    cmp rax,rdi
    je s160_finish
s160_site LABEL BYTE
    DB 0fh,1fh,84h,0,0,0,0,0
s160_loop LABEL NEAR
    mov rcx,[rbx]
    mov rdx,rbp
    call s160_visit
    add rbx,8
    cmp rbx,rdi
    jne s160_loop
s160_finish LABEL NEAR
    add rsp,20h
    pop rdi
    pop rbp
    pop rbx
    ret
s160_fixture ENDP
s160_end LABEL BYTE

s160_probe PROC FRAME
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
    mov rcx,[s160_input]
    mov rbx,[s160_input+8]
    mov rax,rbx
    mov rdi,[s160_input+16]
    mov rbp,[s160_input+24]
    mov rdx,12345678h
    mov r8,123456789h
    mov r9,987654321h
    mov r10,456789012h
    mov r11,567890123h
    mov rsi,11223344h
    mov r12,12121212h
    mov r13,13131313h
    mov r14,14141414h
    mov r15,15151515h
    fxsave64 [s160_fx_before]
    mov [s160_rsp_before],rsp
    call cmf_refresh_spread_bridge_v160
    mov [s160_rsp_after],rsp
    fxsave64 [s160_fx_after]
    mov [s160_regs],rax
    mov [s160_regs+8],rcx
    mov [s160_regs+16],rdx
    mov [s160_regs+24],rbx
    mov [s160_regs+32],rbp
    mov [s160_regs+40],rsi
    mov [s160_regs+48],rdi
    mov [s160_regs+56],r8
    mov [s160_regs+64],r9
    mov [s160_regs+72],r10
    mov [s160_regs+80],r11
    mov [s160_regs+88],r12
    mov [s160_regs+96],r13
    mov [s160_regs+104],r14
    mov [s160_regs+112],r15
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
s160_probe ENDP
END
