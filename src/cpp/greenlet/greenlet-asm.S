/*
 * This file is part of cgreenlet. CGreenlet is free software available
 * under the terms of the MIT license. Consult the file LICENSE that was
 * shipped together with this source file for the exact licensing terms.
 *
 * Copyright (c) 2012 by the cgreenlet authors. See the file AUTHORS for a
 * full list.
 */

# define cdecl(s) s

.text

.globl cdecl(asm_greenlet_savecontext)
.globl cdecl(asm_greenlet_switchcontext)
.globl cdecl(asm_greenlet_newstack)

cdecl(asm_greenlet_savecontext):
    .cfi_startproc
    .cfi_def_cfa %rsp,8
    movq (%rsp), %rax
    movq %rax, (%rdi)
    movq %rsp, %rax
    addq $0x8, %rax
    movq %rax, 0x8(%rdi)
    movq %rbp, 0x10(%rdi)
    movq %rbx, 0x18(%rdi)
    movq %r12, 0x20(%rdi)
    movq %r13, 0x28(%rdi)
    movq %r14, 0x30(%rdi)
    movq %r15, 0x38(%rdi)
    movq $0x0, %rax
    ret
    .cfi_endproc

cdecl(asm_greenlet_switchcontext):
    .cfi_startproc
    .cfi_def_cfa %rsp,0
    movq 0x8(%rdi), %rsp
    movq 0x10(%rdi), %rbp
    movq 0x18(%rdi), %rbx
    movq 0x20(%rdi), %r12
    movq 0x28(%rdi), %r13
    movq 0x30(%rdi), %r14
    movq 0x38(%rdi), %r15
    testq %rsi, %rsi
    jnz 1f
    movq $0x1, %rax
    jmpq *(%rdi)
1:
    pushq (%rdi)
    .cfi_def_cfa %rsp,8
    movq %rdx, %rdi
    callq *%rsi
    movq $0x1, %rax
    ret
    .cfi_endproc

cdecl(asm_greenlet_newstack):
    .cfi_startproc
    .cfi_def_cfa %rsp,8
    movq %rdi, %rsp
    movq %rdx, %rdi
    pushq $0x0
    jmpq *%rsi
    .cfi_endproc
