; Authored 1.62 clamp and attempt-budget consumer. Never game execution.
; The 70-byte clamp matches the real image, including its final near JE.
option casemap:none
PUBLIC cap162_fixture,cap162_site,cap162_end
.code
cap162_fixture PROC FRAME
    push rbp
    .pushreg rbp
    sub rsp,180h
    .allocstack 180h
    .endprolog
    lea rbp,[rsp+100h]
    mov [rsp+70h],rcx
    ALIGN 2
    nop                         ; odd clamp start +19 = even two-byte site
    cmp qword ptr [rsp+70h],4
    mov ecx,0FFFFFFFFh
    mov [rbp+50h],rsi
    mov byte ptr [rbp+78h],1
cap162_site LABEL NEAR
    jae SHORT upper162
    mov qword ptr [rsp+78h],4
    jmp SHORT done162
upper162:
    cmp qword ptr [rsp+70h],10
    jbe SHORT middle162
    mov qword ptr [rsp+78h],10
    jmp SHORT done162
middle162:
    mov rax,[rsp+70h]
    mov [rsp+78h],rax
    test rax,rax
    je zero162
done162:
    xor edx,edx
attempt162:
    inc rdx
    cmp rdx,[rsp+78h]
    jae finished162
    jmp attempt162
finished162:
    mov rax,rdx
    add rsp,180h
    pop rbp
    ret
    db (08A8h - ($ - done162)) dup(90h)
zero162:
    xor eax,eax
    add rsp,180h
    pop rbp
    ret
cap162_end LABEL NEAR
cap162_fixture ENDP
END
