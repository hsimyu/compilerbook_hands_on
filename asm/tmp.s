.intel_syntax noprefix
.globl main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 208
# FUNCCALL
  call foo
  pop rax
  pop rax
  mov rsp, rbp
  pop rbp
  ret
