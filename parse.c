#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Token *token;

Type *new_type(TypeKind kind) {
  Type *type = malloc(sizeof(Type));
  type->kind = kind;
  return type;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->kind = kind;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_ident(Token *t) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_IDENT;
  char *name = malloc((t->len + 1) * sizeof(char));
  strncpy(name, t->str, t->len);
  name[t->len] = '\0';
  node->name = name;
  return node;
}

Node *new_node_if(Node *cond, Node *then, Node *els) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_IF;
  node->cond = cond;
  node->then = then;
  node->els = els;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_NUM;
  node->val = val;
  return node;
}

static LVar *lvar_new(Token *tok, Type *type) {
  LVar *lvar = malloc(sizeof(LVar));
  lvar->name = tok->str;
  lvar->len = tok->len;
  lvar->type = type;
  return lvar;
}

static void lvar_add(Function *func, LVar *new) {
  int size;
  if (new->type->kind == TY_INT)
    size = 4;
  else
    size = 8;

  if (func->lvar == NULL) {
    new->offset = size;
    func->lvar = new;
    return;
  }
  new->offset = func->lvar->offset + size;
  new->next = func->lvar;
  func->lvar = new;
}

Function *new_func(Token *t) {
  Function *func = malloc(sizeof(Function));
  char *name = malloc((t->len + 1) * sizeof(char));
  strncpy(name, t->str, t->len);
  name[t->len] = '\0';
  func->name = name;
  func->lvar = NULL;
  func->lval_len = 0;
  func->arg_len = 0;
  func->code = new_vector();
  return func;
}

Node *stmt();
Node *assign();
Node *equality();
Node *add();
Node *mul();
Node *primary();

static int consume(int ty) {
  if (token->ty != ty) {
    return 0;
  }
  token = token->next;
  return 1;
}

Function *funcs;
static Function *func;
// program: stmt program | e
void program() {
  token = tokens;
  func = malloc(sizeof(Function));
  funcs = func;
  while (token != NULL) {
    consume(TK_INT);
    Token *t = token;
    Function *f = new_func(t);
    func->next = f;
    func = func->next;
    token = token->next;
    consume('(');
    if (!consume(')')) {
      while (1) {
        consume(TK_INT);
        t = token;
        if (find_lvar(f->lvar, t) == NULL) {
          Type *type = new_type(TY_INT);
          lvar_add(f, lvar_new(t, type));
          f->lval_len++;
          f->arg_len++;
        }
        token = token->next;
        if (!consume(',')) {
          break;
        }
      }
      if (!consume(')')) {
        t = token;
        error_at("expected ')'", t->str);
      }
    }
    consume('{');
    while (!consume('}')) {
      vec_push(f->code, stmt());
    }
  }
  funcs = funcs->next;
}

Node *expression();

/*
 * declaration-specifiers:
 *     storage-class-specifier declaration-specifiers_opt
 *     type-specifier declaration-specifiers_opt
 *     type-qualifier declaration-specifiers_opt
 *     function-specifier declaration-specifiers_opt
 *     alignment-specifier declaration-specifiers_opt
 */
Type *decl_specifiers() {
  if (consume(TK_INT)) {
    return new_type(TY_INT);
  }
  error_at("expected int", token->str);
}

/*
 * declarator:
 *     pointer_opt direct-declarator
 */
void declarator(Type *to) {
  Type *type = to;
  while (consume('*')) {
    type = new_type(TY_PTR);
    type->ptrto = to;
    to = type;
  }

  // identifier
  if (token->ty != TK_IDENT) {
    error_at("expected token", token->str);
  }

  Function *f = func;
  lvar_add(f, lvar_new(token, type));
  f->lval_len++;

  token = token->next;
}

/**
 * declaration:
 *     declaration-specifiers init-declarator-list_opt ";"
 *     static_assert-declaration
 */
Node *declaration() {
  Type *type = decl_specifiers();
  declarator(type);
  consume(';');

  return NULL;
}

static Node *if_stmt() {
  consume(TK_IF);
  if (!consume('(')) {
    error_at("expected '(' after 'if' token", token->str);
  }
  Node *exp = expression();
  if (!consume(')')) {
    error_at("expected ')'", token->str);
  }
  Node *then = stmt();
  if (consume(TK_ELSE)) {
    return new_node_if(exp, then, stmt());
  }
  return new_node_if(exp, then, NULL);
}

static Node *while_stmt() {
  consume(TK_WHILE);
  if (!consume('(')) {
    error_at("expected '(' after 'while' token", token->str);
  }
  Node *exp = expression();
  if (!consume(')')) {
    error_at("expected ')'", token->str);
  }
  return new_node(ND_WHILE, exp, stmt());
}

/*
 * compound-statement:
 *     "{" block-item-list_opt "}"
 *
 * block-item-list:
 *     block-item
 *     block-item-list
 *
 * block-item:
 *     declaration // TODO
 *     statement
 */
static Node *compound_stmt() {
  consume('{');
  Vector *stmts = new_vector();
  while (!consume('}')) {
    vec_push(stmts, stmt());
  }
  Node *node = malloc(sizeof(Node));
  node->kind = ND_BLOCK;
  node->stmts = stmts;
  return node;
}

static Node *return_stmt() {
  consume(TK_RET);
  Node *node = malloc(sizeof(Node));
  node->kind = ND_RET;
  node->lhs = expression();
  if (!consume(';')) {
    error_at("expected ';'", token->str);
  }
  return node;
}

static Node *expression_stmt() {
  Node *node = expression();
  if (!consume(';')) {
    error_at("expected ';'", token->str);
  }
  return node;
}

/*
 * statement:
 *     selection-statement
 *     compound-statement
 *     expression-statement
 *     jump-statement
 *
 * selection-statement:
 *     "if" "(" expression ")" statement
 *
 * iteration-statement:
 *     "while" "(" expression ")" statement
 *
 * expression-statement:
 *     expression ";"
 *
 * jump-statement:
 *     "return" expression_opt ";"
 */
Node *stmt() {
  switch (token->ty) {
  case TK_IF:
    return if_stmt();
  case TK_WHILE:
    return while_stmt();
  case '{':
    return compound_stmt();
  case TK_RET:
    return return_stmt();
  case TK_INT:
    return declaration();
  default:
    return expression_stmt();
  }
}

/*
 * expression:
 *     expression "," assign | assign
 *
 * expression:
 *     assign expression'
 *
 * expression':
 *     "," expression'
 */
Node *expression() {
  Node *node = assign();
  while (1) {
    if (consume(',')) {
      node = new_node(',', node, expression());
    } else {
      return node;
    }
  }
}

// assign: equality "=" assign | equality
Node *assign() {
  Node *node = equality();
  if (consume('=')) {
    node = new_node(ND_ASSIGN, node, assign());
  }
  return node;
}

/*
 * relational-expression:
 *     shift-expression
 *     relational-expression "<" shift-expression
 *     relational-expression ">" shift-expression
 *     relational-expression "<=" shift-expression
 *     relational-expression ">=" shift-expression
 */
Node *relational() {
  // relational: add relational'
  // relational': "<=" add relational' | ">=" add relational | e
  Node *node = add();
  while (1) {
    if (consume('<')) {
      node = new_node(ND_LT, node, add());
      continue;
    }
    if (consume('>')) {
      node = new_node(ND_GT, node, add());
      continue;
    }
    if (consume(TK_LE)) {
      node = new_node(ND_LE, node, add());
      continue;
    }
    if (consume(TK_GE)) {
      node = new_node(ND_GE, node, add());
      continue;
    }
    return node;
  }
}

/*
 * equality-expression:
 *     relational-expression
 *     equality-expression "==" relational-expression
 *     equality-expression "!=" relational-expression
 */
Node *equality() {
  /*
   * Modified syntax for removing left recursion
   *
   * equality:
   *     relational equality'
   * equality':
   *     "==" relational equality' | "!=" relational equality | e
   */
  Node *node = relational();
  while (1) {
    if (consume(TK_EQ)) {
      node = new_node(ND_EQ, node, relational());
      continue;
    }
    if (consume(TK_NE)) {
      node = new_node(ND_NE, node, relational());
      continue;
    }
    return node;
  }
}

Node *function_call(Token *t) {
  Node *node = malloc(sizeof(Node));
  node->kind = ND_CALL;
  char *name = malloc((t->len + 1) * sizeof(char));
  strncpy(name, t->str, t->len);
  name[t->len] = '\0';
  node->name = name;
  node->args = new_vector();
  if (consume(')')) {
    return node;
  }
  while (1) {
    vec_push(node->args, assign());
    if (!consume(',')) {
      break;
    }
  }
  if (!consume(')')) {
    error_at("expected ')'", token->str);
  }
  return node;
}

/**
 * primary-expression:
 *     identifier
 *     constant
 *     string-literal // TODO
 *     "(" expression ")"
 *     generic-selection // TODO
 */
Node *primary() {
  // "(" expression ")"
  if (consume('(')) {
    Node *node = expression();
    if (!consume(')')) {
      error_at("expected ')'", token->str);
    }
    return node;
  }

  // identifier
  Token *t = token;
  if (consume(TK_IDENT)) {
    if (consume('(')) {
      return function_call(t);
    }
    Node *node = new_node_ident(t);
    Function *f = func;
    LVar *lvar = find_lvar(f->lvar, t);
    if (lvar == NULL) {
      error_at("'' was not declared in this scope", t->str);
    }
    node->lvar = lvar;
    return node;
  }

  // constant
  if (consume(TK_NUM)) {
    Node *node = new_node_num(t->val);
    return node;
  }

  error_at("expected number, ident or '('", t->str);
}

/**
 * unary-expression:
 *     postfix-expression
 *     "++" unary-expression // TODO
 *     "-- unary-expression // TODO
 *     unary-operator cast-expression
 *     "sizeof" unary-expression // TODO
 *     "sizeof" "(" type-name ")" // TODO
 *     "_Alignof" "(" type-name ")" // TODO
 */
Node *unary() {
  if (consume('+')) {
    return primary();
  }
  if (consume('-')) {
    return new_node(ND_SUB, new_node_num(0), primary());
  }
  if (consume('&')) {
    return new_node(ND_ADDR, primary(), NULL);
  }
  if (consume('*')) {
    return new_node(ND_DEREF, primary(), NULL);
  }
  return primary();
}

/**
 * multiplicative-expression:
 *     cast-expression
 *     multiplicative-expression "*" cast-expression
 *     multiplicative-expression "/" cast-expression
 *     multiplicative-expression "%" cast-expression // TODO
 */
Node *mul() {
  Node *node = unary();
  while (1) {
    if (consume('*')) {
      node = new_node(ND_MUL, node, unary());
    } else if (consume('/')) {
      node = new_node(ND_DIV, node, unary());
    } else {
      return node;
    }
  }
}

// add: add "+" mul | add "-" mul | mul
//
// add: mul add'
// add': "+" mul add' | "-" mul add' | e
Node *add() {
  Node *node = mul();
  while (1) {
    if (consume('+')) {
      node = new_node(ND_ADD, node, mul());
    } else if (consume('-')) {
      node = new_node(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}
