#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pos = 0;

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

int consume(int ty) {
  Token *t = tokens->data[pos];
  if (t->ty != ty) {
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
    Token *t = tokens->data[pos];
    Function *f = new_func(t->name);
    vec_push(funcs, f);
    pos++;
    consume('(');
    if (!consume(')')) {
      while (1) {
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
  if (consume(TK_IF)) {
    if (!consume('(')) {
      Token *t = tokens->data[pos];
      error_at("expected '(' after 'if' token", t->input);
    }
    Node *exp = expression();
    if (!consume(')')) {
      Token *t = tokens->data[pos];
      error_at("expected ')'", t->input);
    }
    return new_node(ND_IF, exp, stmt());
  }

  if (consume(TK_WHILE)) {
    if (!consume('(')) {
      Token *t = tokens->data[pos];
      error_at("expected '(' after 'while' token", t->input);
    }
    Node *exp = expression();
    if (!consume(')')) {
      Token *t = tokens->data[pos];
      error_at("expected ')'", t->input);
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
  if (consume('{')) {
    Vector *stmts = new_vector();
    while (!consume('}')) {
      vec_push(stmts, stmt());
    }
    Node *node = malloc(sizeof(Node));
    node->ty = ND_BLOCK;
    node->stmts = stmts;
    return node;
  }

  if (consume(TK_RET)) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_RET;
    node->lhs = expression();
    if (!consume(';')) {
      Token *t = tokens->data[pos];
      error_at("expected ';'", t->input);
    }
    return node;
  } else {
    Node *node = expression();
    if (!consume(';')) {
      Token *t = tokens->data[pos];
      error_at("expected ';'", t->input);
    }
    return node;
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
      int *offset = malloc(sizeof(int));
      *offset = f->lval_len + 1;
      map_put(f->lval, t->name, offset);
      f->lval_len++;
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

// mul: mul "*" term | mul "/" term | term
//
// mul: term mul'
// mul': "*" term mul' | "/" term mul' | e
Node *mul() {
  Node *node = primary();
  while (1) {
    if (consume('*')) {
      node = new_node('*', node, primary());
    } else if (consume('/')) {
      node = new_node('/', node, primary());
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
