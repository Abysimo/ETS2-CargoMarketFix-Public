OPTION CASEMAP:NONE
EXTERN cmf_refresh_spread_active:DWORD
PUBLIC cmf_refresh_spread_bridge
PUBLIC cmf_refresh_spread_bridge_end
.code
; Entered only at normal RE002 sweep padding, through CALL ending at 003EC64C.
; RAX=N, RBX=begin, RDI=end, RSI=economy. These bounds are loaded by the engine.
; RFLAGS are dead: next original code is MOV/MOV/CALL; RE001 defines flags before
; consuming any. RCX is dead (overwritten at 003EC650), used for JRCXZ selection.
; All other GPRs except intentionally narrowed RBX/RDI are preserved.
; No SIMD/FP, TLS, allocation, query observer, or per-company helper.
cmf_refresh_spread_bridge PROC FRAME
    ; Lease begins before ANY new stack access, including a guard-page fault.
    lock inc DWORD PTR [cmf_refresh_spread_active]
    sub rsp,88h
    .allocstack 88h
    mov [rsp+40h],rbx
    .savereg rbx,40h
    mov [rsp+48h],rdi
    .savereg rdi,48h
    .endprolog
    mov [rsp],rax
    mov [rsp+10h],rdx
    mov [rsp+18h],r8
    mov [rsp+20h],r9
    mov [rsp+28h],r10
    mov [rsp+30h],r11
    mov ecx,1                 ; unexpected bounds retain the full original loop
    ; An unexpected/nonrepresentable container retains the original full range.
    mov r8,rax
    shr r8,61
    jnz spread_done
    test rax,rax
    jz spread_done
    mov r8,rax
    shl r8,3
    add r8,rbx
    jc spread_done
    cmp r8,rdi
    jne spread_done
    ; Same minute field read by this caller immediately before sweep setup.
    ; If a fault unwinds this new frame the lease remains nonzero: containment,
    ; never unsafe restoration/free, never swallowed or retried.
    mov eax,DWORD PTR [rsi+15Ch]
    xor edx,edx
    mov r8d,60
    div r8
    mov r9,rdx                 ; bucket
    mov rax,[rsp]              ; N
    xor edx,edx
    div r8                     ; q=N/60, remainder r
    mov r10,rax                ; length=q+(bucket<r)
    imul rax,r9                ; start=bucket*q+min(bucket,r), <=N
    mov r11,rdx
    cmp r9,rdx
    cmovb r11,r9
    add rax,r11
    cmp r9,rdx
    jae spread_bounds
    inc r10
spread_bounds:
    lea rbx,[rbx+rax*8]
    lea rdi,[rbx+r10*8]
    mov rcx,r10                ; zero-length bucket -> original sweep-end branch
spread_done:
    mov rax,[rsp]
    mov rdx,[rsp+10h]
    mov r8,[rsp+18h]
    mov r9,[rsp+20h]
    mov r10,[rsp+28h]
    mov r11,[rsp+30h]
    lock dec DWORD PTR [cmf_refresh_spread_active]
    add rsp,88h
    ret
cmf_refresh_spread_bridge ENDP
cmf_refresh_spread_bridge_end LABEL BYTE
END
