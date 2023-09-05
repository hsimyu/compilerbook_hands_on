.intel_syntax noprefix
.globl main
main:
  push 20
  push 3
  pop rdi
  pop rax
  add rax, rdi
  push rax
  push 40
  pop rdi
  pop rax
  sub rax, rdi
  push rax
  push 0
  push 4
  pop rdi
  pop rax
  sub rax, rdi
  push rax
  pop rdi
  pop rax
  cmp rdi, rax
  setle al
  movzb rax, al
  push rax
  pop rax
  ret
