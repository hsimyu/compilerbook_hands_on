.intel_syntax noprefix
.globl main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 208
  push 1
  pop rax
  push 2
  pop rax
  mov rsp, rbp
  pop rbp
  ret
