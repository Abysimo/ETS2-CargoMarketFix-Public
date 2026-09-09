OPTION CASEMAP:NONE
EXTERN spread_test_visit:PROC
EXTERN cmf_refresh_spread_bridge:PROC
EXTERN spread_registers:QWORD
EXTERN spread_fx_before:BYTE
EXTERN spread_fx_after:BYTE
EXTERN spread_rsp_before:QWORD
EXTERN spread_rsp_after:QWORD
PUBLIC spread_fixture
PUBLIC spread_site
PUBLIC spread_loop
PUBLIC spread_finish
PUBLIC spread_fixture_end
PUBLIC spread_probe
.code
spread_fixture PROC FRAME
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    sub rsp,20h
    .allocstack 20h
    .endprolog
    mov rbx,rcx
    mov rax,rdx
    mov rsi,r8
    lea rdi,[rbx+rax*8]
    cmp rbx,rdi
    je spread_finish
spread_site LABEL BYTE
    DB 66h,0fh,1fh,84h,0,0,0,0,0
spread_loop LABEL NEAR
    mov rcx,[rbx]
    mov rdx,rsi
    call spread_test_visit
    add rbx,8
    cmp rbx,rdi
    jne spread_loop
spread_finish LABEL NEAR
    add rsp,20h
    pop rdi
    pop rsi
    pop rbx
    ret
spread_fixture ENDP
spread_fixture_end LABEL BYTE

spread_probe PROC FRAME
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    sub rsp,20h
    .allocstack 20h
    .endprolog
    mov rsi,rcx
    mov rax,60
    mov rbx,10000h
    mov rdi,101e0h
    mov rdx,12345678h
    mov r8,123456789h
    mov r9,987654321h
    mov r10,456789012h
    mov r11,567890123h
    fxsave64 [spread_fx_before]
    mov [spread_rsp_before],rsp
    call cmf_refresh_spread_bridge
    mov [spread_rsp_after],rsp
    fxsave64 [spread_fx_after]
    mov [spread_registers],rax
    mov [spread_registers+8],rcx
    mov [spread_registers+16],rdx
    mov [spread_registers+24],r8
    mov [spread_registers+32],r9
    mov [spread_registers+40],r10
    mov [spread_registers+48],r11
    mov [spread_registers+56],rsi
    mov [spread_registers+64],rbx
    mov [spread_registers+72],rdi
    add rsp,20h
    pop rdi
    pop rsi
    pop rbx
    ret
spread_probe ENDP
END
