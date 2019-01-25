#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

Token *new_token(char *p) {
    Token *token = malloc(sizeof(Token));
    token->ty = *p;
    token->input = p;
    return token;
}

Token *new_token_num(char *p, char **endptr) {
    Token *token = malloc(sizeof(Token));
    token->ty = TK_NUM;
    token->input = p;
    token->val = strtol(p, endptr, 10);
    return token;
}

Token *new_token_ty(char *p, int ty) {
    Token *token = malloc(sizeof(Token));
    token->ty = ty;
    token->input = p;
    return token;
}


Vector *tokens;
int pos = 0;

void tokenize(char *p) {
    tokens = new_vector();
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if ('a' <= *p && *p <= 'z') {
            vec_push(tokens, new_token_ty(p, TK_IDENT));
            p++;
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == ';') {
            vec_push(tokens, new_token(p));
            p++;
            continue;
        }

        if (isdigit(*p)) {
            vec_push(tokens, new_token_num(p, &p));
            continue;
        }

        fprintf(stderr, "Cannot tokenize: %s\n", p);
        exit(1);
    }

    vec_push(tokens, new_token_ty(p, TK_EOF));
}

Node *new_node(int ty, Node *lhs, Node *rhs) {
    Node *node = malloc(sizeof(Node));
    node->ty = ty;
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_node_num(int val) {
    Node *node = malloc(sizeof(Node));
    node->ty = ND_NUM;
    node->val = val;
    return node;
}

Node *code[100];

Node *stmt();
Node *assign();
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

// program: stmt program | e
void program() {
    int i = 0;
    while (((Token *)tokens->data[pos])->ty != TK_EOF) {
        code[i++] = stmt();
    }
    code[i] = NULL;
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

// assign: add "=" assign | add
Node *assign() {
    Node * node = add();
    if (consume('=')) {
        node = new_node('=', node, assign());
    }
    return node;
}

// term: "(" add ")" | num
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
