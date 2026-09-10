OPTION CASEMAP:NONE
EXTERN cmf_refresh_spread_active:DWORD
EXTERN cmf_refresh_spread_bucket_count:DWORD
PUBLIC cmf_refresh_spread_bridge_v160
PUBLIC cmf_refresh_spread_bridge_v160_end
.code
; Candidate ABI only: RCX=N, RAX=begin, RBX=cursor, RDI=end, RBP=economy.
; RCX becomes bucket length for the owned JRCXZ. Only RBX/RDI are narrowed.
; RFLAGS are dead: original MOV/MOV/CALL reaches a callee XOR before any use.
; Same static-unwind / fault-retained lease contract as the legacy bridge.
cmf_refresh_spread_bridge_v160 PROC FRAME
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
    mov [rsp+38h],rcx       ; preserve input N before producing output RCX
    mov rax,rcx
    mov ecx,1              ; invalid bounds leave original full loop unchanged
    mov r8,rax
    shr r8,61
    jnz v160_done
    test rax,rax
    jz v160_done
    mov r8,rax
    shl r8,3
    add r8,rbx
    jc v160_done
    cmp r8,rdi
    jne v160_done
    mov eax,DWORD PTR [rbp+19Ch]
    xor edx,edx
    mov r8d,DWORD PTR [cmf_refresh_spread_bucket_count] ; immutable installed startup period
    div r8
    mov r9,rdx
    mov rax,[rsp+38h]
    xor edx,edx
    div r8
    mov r10,rax
    imul rax,r9
    mov r11,rdx
    cmp r9,rdx
    cmovb r11,r9
    add rax,r11
    cmp r9,rdx
    jae v160_bounds
    inc r10
v160_bounds:
    lea rbx,[rbx+rax*8]
    lea rdi,[rbx+r10*8]
    mov rcx,r10
v160_done:
    mov rax,[rsp]
    mov rdx,[rsp+10h]
    mov r8,[rsp+18h]
    mov r9,[rsp+20h]
    mov r10,[rsp+28h]
    mov r11,[rsp+30h]
    lock dec DWORD PTR [cmf_refresh_spread_active]
    add rsp,88h
    ret
cmf_refresh_spread_bridge_v160 ENDP
cmf_refresh_spread_bridge_v160_end LABEL BYTE
END
