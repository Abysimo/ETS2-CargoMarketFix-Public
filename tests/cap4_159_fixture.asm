; Authored local fixture: the recompiled stack-budget clamp, not game execution.
option casemap:none
PUBLIC cap159_fixture,cap159_site,cap159_end
.code
cap159_fixture PROC FRAME
    push rbp
    .pushreg rbp
    sub rsp,180h
    .allocstack 180h
    .endprolog
    lea rbp,[rsp+100h]
    mov [rsp+68h],rcx
    ALIGN 2
    nop
    cmp qword ptr [rsp+68h],4
    mov ecx,0FFFFFFFFh
    mov [rbp+20h],rsi
    mov byte ptr [rbp+48h],1
cap159_site LABEL NEAR
    jae SHORT upper159
    mov qword ptr [rsp+70h],4
    jmp SHORT done159
upper159:
    cmp qword ptr [rsp+68h],10
    jbe SHORT middle159
    mov qword ptr [rsp+70h],10
    jmp SHORT done159
middle159:
    mov rax,[rsp+68h]
    mov [rsp+70h],rax
    test rax,rax
    je zero159
done159:
    mov rax,[rsp+70h]
    add rsp,180h
    pop rbp
    ret
    db 160 dup(90h)
zero159:
    xor eax,eax
    add rsp,180h
    pop rbp
    ret
cap159_end LABEL NEAR
cap159_fixture ENDP
END
