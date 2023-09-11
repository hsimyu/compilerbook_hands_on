.intel_syntax noprefix
.globl main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 208
# ASSIGN BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
  push 0
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
# ASSIGN END
  pop rax
# IFELSE
# LVAR BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
  push 1
  pop rdi
  pop rax
# EQ
  cmp rax, rdi
  sete al
  movzb rax, al
  push rax
  pop rax
  cmp rax, 0
  je .Lelse0
# RETURN
  push 42
  pop rax
  mov rsp, rbp
  pop rbp
  ret
  jmp .Lend0
.Lelse0:
# RETURN
  push 35
  pop rax
  mov rsp, rbp
  pop rbp
  ret
.Lend0:
  pop rax
  mov rsp, rbp
  pop rbp
  ret
