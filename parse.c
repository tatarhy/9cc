#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int pos = 0;

Node *new_node(int ty, Node *lhs, Node *rhs) {
  Node *node = malloc(sizeof(Node));
  node->ty = ty;
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

Node *new_node_ident(char *name) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_IDENT;
  node->name = name;
  return node;
}

Node *new_node_num(int val) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_NUM;
  node->val = val;
  return node;
}

Function *new_func(char *name) {
  Function *func = malloc(sizeof(Function));
  func->name = name;
  func->lval = new_map();
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

static Token *gettok() {
  return tokens->data[pos];
}

static int consume(int ty) {
  if (gettok()->ty != ty) {
    return 0;
  }
  pos++;
  return 1;
}

Vector *funcs;
// program: stmt program | e
void program() {
  funcs = new_vector();
  while (((Token *)tokens->data[pos])->ty != TK_EOF) {
    consume(TK_INT);
    Token *t = tokens->data[pos];
    Function *f = new_func(t->name);
    vec_push(funcs, f);
    pos++;
    consume('(');
    if (!consume(')')) {
      while (1) {
        consume(TK_INT);
        t = tokens->data[pos];
        if (map_get(f->lval, t->name) == NULL) {
          int *offset = malloc(sizeof(int));
          *offset = f->lval_len + 1;
          map_put(f->lval, t->name, offset);
          f->lval_len++;
          f->arg_len++;
        }
        pos++;
        if (!consume(',')) {
          break;
        }
      }
      if (!consume(')')) {
        t = tokens->data[pos];
        error_at("expected ')'", t->input);
      }
    }
    consume('{');
    while (!consume('}')) {
      vec_push(f->code, stmt());
    }
  }
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
  Token *t = tokens->data[pos];
  if (consume(TK_INT)) {
    Type *type = malloc(sizeof(Type));
    type->ty = INT;
    return type;
  }
  error_at("expected int", t->input);
}

/*
 * declarator:
 *     pointer_opt direct-declarator
 */
void declarator(Type *type) {
  while (consume('*')) {
    Type *type = malloc(sizeof(Type));
    type->ty = PTR;
    type->ptrto = type;
  }
  // identifier
  Token *t = tokens->data[pos];
  if (t->ty != TK_IDENT) {
    error_at("expected token", t->input);
  }
  pos++;

  Function *f = funcs->data[funcs->len - 1];
  int *offset = malloc(sizeof(int));
  *offset = f->lval_len + 1;
  map_put(f->lval, t->name, offset);
  f->lval_len++;
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
    error_at("expected '(' after 'if' token", gettok()->input);
  }
  Node *exp = expression();
  if (!consume(')')) {
    error_at("expected ')'", gettok()->input);
  }
  return new_node(ND_IF, exp, stmt());
}

static Node *while_stmt() {
  consume(TK_WHILE);
  if (!consume('(')) {
    error_at("expected '(' after 'while' token", gettok()->input);
  }
  Node *exp = expression();
  if (!consume(')')) {
    error_at("expected ')'", gettok()->input);
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
  node->ty = ND_BLOCK;
  node->stmts = stmts;
  return node;
}

static Node *return_stmt() {
  consume(TK_RET);
  Node *node = malloc(sizeof(Node));
  node->ty = ND_RET;
  node->lhs = expression();
  if (!consume(';')) {
    error_at("expected ';'", gettok()->input);
  }
  return node;
}

static Node *expression_stmt() {
  Node *node = expression();
  if (!consume(';')) {
    error_at("expected ';'", gettok()->input);
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
  switch (gettok()->ty) {
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
    node = new_node('=', node, assign());
  }
  return node;
}

// relational: relational "<=" add | relational ">=" add | add
//
// relational: add relational'
// relational': "<=" add relational' | ">=" add relational | e
Node *relational() {
  Node *node = add();
  while (1) {
    Token *t = tokens->data[pos];
    switch (t->ty) {
    case '<':
      pos++;
      node = new_node('<', node, add());
      break;
    case '>':
      pos++;
      node = new_node('>', node, add());
      break;
    case TK_LE:
      pos++;
      node = new_node(ND_LE, node, add());
      break;
    case TK_GE:
      pos++;
      node = new_node(ND_GE, node, add());
      break;
    default:
      return node;
    }
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
    Token *t = tokens->data[pos];
    switch (t->ty) {
    case TK_EQ:
      pos++;
      node = new_node(ND_EQ, node, relational());
      break;
    case TK_NE:
      pos++;
      node = new_node(ND_NE, node, relational());
      break;
    default:
      return node;
    }
  }
}

Node *function_call(Token *t) {
  Node *node = malloc(sizeof(Node));
  node->ty = ND_CALL;
  node->name = t->name;
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
    Token *t = tokens->data[pos];
    error_at("expected ')'", t->input);
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
      Token *t = tokens->data[pos];
      error_at("expected ')'", t->input);
    }
    return node;
  }

  // identifier
  Token *t = tokens->data[pos];
  if (t->ty == TK_IDENT) {
    pos++;
    if (consume('(')) {
      return function_call(t);
    }
    Node *node = new_node_ident(t->name);
    Function *f = funcs->data[funcs->len - 1];
    if (map_get(f->lval, t->name) == NULL) {
      error_at("'' was not declared in this scope", t->input);
    }
    return node;
  }

  // constant
  if (t->ty == TK_NUM) {
    Node *node = new_node_num(t->val);
    pos++;
    return node;
  }

  error_at("expected number, ident or '('", t->input);
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
    return new_node('-', new_node_num(0), primary());
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
      node = new_node('*', node, unary());
    } else if (consume('/')) {
      node = new_node('/', node, unary());
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
      node = new_node('+', node, mul());
    } else if (consume('-')) {
      node = new_node('-', node, mul());
    } else {
      return node;
    }
  }
}
