#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
Function *f_now;
int lcnt = 1;

/**
 * Generate lvalue code
 */
void gen_lval(Node *node) {
  if (node->ty != ND_IDENT) {
    fprintf(stderr, "lvalue is not variable\n");
    exit(1);
  }
  // resolve variable address and push result to stack top
  int *idx = map_get(f_now->lval, node->name);
  int offset = *idx * 8;
  printf("    mov rax, rbp\n");
  printf("    sub rax, %d\n", offset);
  printf("    push rax\n");
}

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("    push %d\n", node->val);
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    printf("    pop rax\n");
    printf("    mov rax, [rax]\n");
    printf("    push rax\n");
    return;
  }

  if (node->ty == ND_CALL) {
    for (int i = 0; i < node->args->len; i++) {
      gen(node->args->data[i]);
    }
    for (int i = node->args->len - 1; i >= 0; i--) {
      printf("    pop %s\n", regs[i]);
    }
    printf("    call %s\n", node->name);
    printf("    push rax\n");
    return;
  }

  if (node->ty == ND_RET) {
    gen(node->lhs);
    printf("    pop rax\n");
    printf("    mov rsp, rbp\n");
    printf("    pop rbp\n");
    printf("    ret\n");
    return;
  }

  if (node->ty == ND_IF) {
    // evaluate conditional expression
    gen(node->lhs);
    printf("    pop rax\n");
    printf("    cmp rax, 0\n");

    // FIXME?
    // push expression result to stack
    // because all statement pop top of stack at end
    printf("    push rax\n");

    // jump to end of if statement if condition is falth
    printf("    je .Lend%d\n", lcnt);

    // pop top of stack pushed previous
    printf("    pop rax\n");
    gen(node->rhs);
    printf(".Lend%d:\n", lcnt);
    lcnt++;
    return;
  }

  if (node->ty == ND_BLOCK) {
    Vector *stmts = node->stmts;
    for (int i = 0; i < stmts->len; i++) {
      gen(stmts->data[i]);
    }
    return;
  }

  if (node->ty == '=') {
    gen_lval(node->lhs);
    gen(node->rhs);

    printf("    pop rdi\n");
    printf("    pop rax\n");
    printf("    mov [rax], rdi\n");
    printf("    push rdi\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("    pop rdi\n");
  printf("    pop rax\n");
  switch (node->ty) {
  case '+':
    printf("    add rax, rdi\n");
    break;
  case '-':
    printf("    sub rax, rdi\n");
    break;
  case '*':
    printf("    mul rdi\n");
    break;
  case '/':
    // Clear rdx because of rdx:rax is dividend
    printf("    mov rdx, 0\n");
    printf("    div rdi\n");
    break;
  case ND_EQ:
    printf("    cmp rdi, rax\n");
    printf("    sete al\n");
    printf("    movzb rax, al\n");
    break;
  case ND_NE:
    printf("    cmp rdi, rax\n");
    printf("    setne al\n");
    printf("    movzb rax, al\n");
    break;
  }

  printf("    push rax\n");
}

void gen_func(Function *f) {
  f_now = f;
  printf("%s:\n", f->name);

  // prologue
  printf("    push rbp\n");
  printf("    mov rbp, rsp\n");
  printf("    sub rsp, %d\n", f->lval_len * 8);

  for (int i = 0; i < f->arg_len; i++) {
    printf("    mov QWORD PTR [rbp-%d], %s\n", (i + 1) * 8, regs[i]);
  }

  for (int i = 0; i < f->code->len; i++) {
    gen(f->code->data[i]);

    printf("    pop rax\n");
  }

  // epilogue
  printf("    mov rsp, rbp\n");
  printf("    pop rbp\n");
  printf("    ret\n");
}

void gen_amd64() {
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");

  for (int i = 0; i < funcs->len; i++) {
    gen_func(funcs->data[i]);
  }
}
