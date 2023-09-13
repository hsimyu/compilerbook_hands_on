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
# FOR
# ASSIGN BEGIN
  mov rax, rbp
  sub rax, 16
  push rax
  push 1
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
# ASSIGN END
.Lbegin0:
# LVAR BEGIN
  mov rax, rbp
  sub rax, 16
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
  push 2
  pop rdi
  pop rax
# LE
  cmp rax, rdi
  setle al
  movzb rax, al
  push rax
  pop rax
  cmp rax, 0
  je .Lend0
# {
# ASSIGN BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
# LVAR BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
# LVAR BEGIN
  mov rax, rbp
  sub rax, 16
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
  pop rdi
  pop rax
  add rax, rdi
  push rax
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
# ASSIGN END
  pop rax
# ASSIGN BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
# LVAR BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
  push 2
  pop rdi
  pop rax
  imul rax, rdi
  push rax
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
# ASSIGN END
  pop rax
# }
  pop rax
# ASSIGN BEGIN
  mov rax, rbp
  sub rax, 16
  push rax
# LVAR BEGIN
  mov rax, rbp
  sub rax, 16
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
  push 1
  pop rdi
  pop rax
  add rax, rdi
  push rax
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
# ASSIGN END
  pop rax
  jmp .Lbegin0
.Lend0:
  pop rax
# RETURN
# LVAR BEGIN
  mov rax, rbp
  sub rax, 8
  push rax
  pop rax
  mov rax, [rax]
  push rax
# LVAR END
  pop rax
  mov rsp, rbp
  pop rbp
  ret
  pop rax
  mov rsp, rbp
  pop rbp
  ret
