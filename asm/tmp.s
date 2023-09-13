.intel_syntax noprefix
.globl main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 208
# FUNCCALL
  mov rax, rsp
  and rax, 15
  cmp rax, 0
  je .Lcallb0
.Lcalla0:
  sub rsp, 8
  call foo
  add rsp, 8
  jmp .Lcallend0
.Lcallb0:
  call foo
.Lcallend0:
  pop rax
  mov rsp, rbp
  pop rbp
  ret
