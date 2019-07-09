#include "9cc.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *argregs[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
static char *argregs8[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
static char *argregs32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
Function *f_now;
int lcnt = 1;

static char *argreg(int r, int size) {
  if (size == 1) {
    return argregs8[r];
  }
  if (size == 4) {
    return argregs32[r];
  }
  assert(size == 8);
  return argregs[r];
}

/**
 * Generate lvalue code
 */
void gen_lval(Node *node) {
  if (node->ty != ND_IDENT) {
    fprintf(stderr, "lvalue is not variable\n");
    exit(1);
  }
  // resolve variable address and push result to stack top
  printf("    movq %%rbp, %%rax\n");
  printf("    subq $%d, %%rax\n", node->offset);
  printf("    pushq %%rax\n");
}

void gen(Node *node) {
  if (node->ty == ND_NUM) {
    printf("    movq $%d, %%rax\n", node->val);
    printf("    pushq %%rax\n");
    return;
  }

  if (node->ty == ND_IDENT) {
    gen_lval(node);
    printf("    popq %%rax\n");
    printf("    movq (%%rax), %%rax\n");
    printf("    pushq %%rax\n");
    return;
  }

  if (node->ty == ND_CALL) {
    for (int i = 0; i < node->args->len; i++) {
      gen(node->args->data[i]);
    }
    for (int i = node->args->len - 1; i >= 0; i--) {
      printf("    popq %s\n", argreg(i, 8));
    }
    printf("    call %s\n", node->name);
    printf("    pushq %%rax\n");
    return;
  }

  if (node->ty == ND_RET) {
    gen(node->lhs);
    printf("    popq %%rax\n");
    printf("    movq %%rbp, %%rsp\n");
    printf("    popq %%rbp\n");
    printf("    ret\n");
    return;
  }

  if (node->ty == ND_IF) {
    // evaluate conditional expression
    gen(node->cond);
    printf("    popq %%rax\n");
    printf("    cmpq $0, %%rax\n");

    // FIXME?
    // push expression result to stack
    // because all statement pop top of stack at end
    printf("    pushq %%rax\n");

    if (node->els == NULL) {
      // jump to end of if statement if condition is falth
      printf("    je .Lend%d\n", lcnt);

      // pop top of stack pushed previous
      printf("    popq %%rax\n");
      gen(node->then);
    } else {
      printf("    je .Lelse%d\n", lcnt);
      gen(node->then);
      printf("    jmp .Lend%d\n", lcnt);
      printf(".Lelse%d:\n", lcnt);
      gen(node->els);
    }
    printf(".Lend%d:\n", lcnt);
    lcnt++;
    return;
  }

  if (node->ty == ND_WHILE) {
    // evaluate conditional expression
    printf(".Lbegin%d:\n", lcnt);
    gen(node->lhs);
    printf("    popq %%rax\n");
    printf("    cmpq $0, %%rax\n");

    // FIXME?
    // push expression result to stack
    // because all statement pop top of stack at end
    printf("    pushq %%rax\n");

    // jump to end of if statement if condition is falth
    printf("    je .Lend%d\n", lcnt);

    // pop top of stack pushed previous
    printf("    popq %%rax\n");
    gen(node->rhs);
    printf("    jmp .Lbegin%d\n", lcnt);
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
    if (node->lhs->ty == ND_DEREF) {
      gen(node->lhs);
    } else {
      gen_lval(node->lhs);
    }

    gen(node->rhs);

    printf("    popq %%rdi\n");
    printf("    popq %%rax\n");
    printf("    movq %%rdi, (%%rax)\n");
    printf("    pushq %%rdi\n");
    return;
  }

  if (node->ty == ND_ADDR) {
    gen_lval(node->lhs);
    return;
  }

  if (node->ty == ND_DEREF) {
    gen(node->lhs);
    printf("    popq %%rax\n");
    printf("    movq (%%rax), %%rax\n");
    printf("    pushq %%rax\n");
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("    popq %%rdi\n");
  printf("    popq %%rax\n");
  switch (node->ty) {
  case '+':
    printf("    addq %%rdi, %%rax\n");
    break;
  case '-':
    printf("    subq %%rdi, %%rax\n");
    break;
  case '*':
    printf("    mulq %%rdi\n");
    break;
  case '/':
    // Clear rdx because of rdx:rax is dividend
    printf("    movq $0, %%rdx\n");
    printf("    divq %%rdi\n");
    break;
  case ND_EQ:
    printf("    cmpq %%rdi, %%rax\n");
    printf("    sete %%al\n");
    printf("    movzbq %%al, %%rax\n");
    break;
  case ND_NE:
    printf("    cmpq %%rdi, %%rax\n");
    printf("    setne %%al\n");
    printf("    movzbq %%al, %%rax\n");
    break;
  case '<':
    printf("    cmpq %%rdi, %%rax\n");
    printf("    setl %%al\n");
    printf("    movzbq %%al, %%rax\n");
    break;
  case '>':
    printf("    cmpq %%rdi, %%rax\n");
    printf("    setg %%al\n");
    printf("    movzbq %%al, %%rax\n");
    break;
  case ND_LE:
    printf("    cmpq %%rdi, %%rax\n");
    printf("    setle %%al\n");
    printf("    movzbq %%al, %%rax\n");
    break;
  case ND_GE:
    printf("    cmpq %%rdi, %%rax\n");
    printf("    setge %%al\n");
    printf("    movzbq %%al, %%rax\n");
    break;
  }

  printf("    pushq %%rax\n");
}

void gen_func(Function *f) {
  f_now = f;
  printf("%s:\n", f->name);

  // prologue
  printf("    pushq %%rbp\n");
  printf("    movq %%rsp, %%rbp\n");
  printf("    subq $%d, %%rsp\n", (f->lvar) ? f->lvar->offset + 8 - 8 : 0);

  for (int i = 0; i < f->arg_len; i++) {
    printf("    movq %s, -%d(%%rbp)\n", argreg(i, 8), (i + 1) * 8);
  }

  for (int i = 0; i < f->code->len; i++) {
    if (f->code->data[i] != NULL) {
      gen(f->code->data[i]);

      printf("    popq %%rax\n");
    }
  }

  // epilogue
  printf("    movq %%rbp, %%rsp\n");
  printf("    popq %%rbp\n");
  printf("    ret\n");
}

void gen_amd64() {
  printf(".global main\n");

  for (Function *func = funcs; func; func = func->next) {
    gen_func(func);
  }
}
