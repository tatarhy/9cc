#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

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
    func->code = new_vector();
    return func;
}

Node *stmt();
Node *assign();
Node *equality();
Node *add();
Node *mul();
Node *term();
void error(char *mes, char *token);

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
        Token *token = tokens->data[pos];
        Function *f = new_func(token->name);
        vec_push(funcs, f);
        pos++;
        consume('(');
        consume(')');
        consume('{');
        while (!consume('}')) {
            vec_push(f->code, stmt());
        }
    }
}

// stmt: assign ";"
Node *stmt() {
    Node *node = assign();
    if (!consume(';')) {
        Token *t = tokens->data[pos];
        error("Token is not ';': %s", t->input);
    }
    return node;
}

// assign: equality "=" assign | equality
Node *assign() {
    Node * node = equality();
    if (consume('=')) {
        node = new_node('=', node, assign());
    }
    return node;
}

// equality: equality "==" add | equality "!=" add | add
//
// equality: add equality'
// equality': "==" add equality' | "!=" add equality | e
Node *equality() {
    Node *node = add();
    while (1) {
        if (consume(TK_EQ)) {
            node = new_node(ND_EQ, node, add());
        } else if (consume(TK_NE)) {
            node = new_node(ND_NE, node, add());
        } else {
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
        error("no close paren matched open paren: %s", t->input);
    }
    return node;
}

// term: "(" add ")" | ident | num
Node *term() {
    if (consume('(')) {
        Node *node = add();
        if (!consume(')')) {
            Token *t = tokens->data[pos];
            error("no close paren matched open paren: %s", t->input);
        }
        return node;
    }

    Token *t = tokens->data[pos];
    if (t->ty == TK_IDENT) {
        pos++;
        if (consume('(')) {
            return function_call(t);
        }
        Node *node = new_node_ident(t->name);
        Function *f = funcs->data[funcs->len - 1];
        int *offset = malloc(sizeof(int));
        *offset = f->lval_len;
        map_put(f->lval, t->name, offset);
        f->lval_len++;
        return node;
    }

    if (t->ty == TK_NUM) {
        Node *node = new_node_num(t->val);
        pos++;
        return node;
    }
    error("token is not number or open paren: %s", t->input);
}

// mul: mul "*" term | mul "/" term | term
//
// mul: term mul'
// mul': "*" term mul' | "/" term mul' | e
Node *mul() {
    Node *node = term();
    while (1) {
        if (consume('*')) {
            node = new_node('*', node, term());
        } else if (consume('/')) {
            node = new_node('/', node, term());
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

void error(char *mes, char *token) {
    fprintf(stderr, mes, token);
    exit(1);
}
