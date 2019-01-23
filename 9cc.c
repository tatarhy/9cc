#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    TK_NUM = 256,
    TK_EOF,
};

typedef struct {
    int ty;
    int val;
    char *input;
} Token;

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

Token *new_token_eof(char *p) {
    Token *token = malloc(sizeof(Token));
    token->ty = TK_EOF;
    token->input = p;
    return token;
}

typedef struct {
    void **data;
    int capacity;
    int len;
} Vector;

Vector *new_vector() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * 16);
    vec->capacity = 16;
    vec->len = 0;
    return vec;
}

void vec_push(Vector *vec, void *elem) {
    if (vec->capacity == vec->len) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

void expect(int line, int expected, int actual) {
    if (expected == actual) {
        return;
    }
    fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
    exit(1);
}

void runtest() {
    Vector *vec = new_vector();
    expect(__LINE__, 0, vec->len);

    for (int i = 0; i < 100; i++) {
        vec_push(vec, (void *)i);
    }

    expect(__LINE__, 100, vec->len);
    expect(__LINE__, 0, (int)vec->data[0]);
    expect(__LINE__, 50, (int)vec->data[50]);
    expect(__LINE__, 99, (int)vec->data[99]);

    printf("OK\n");
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

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
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

    vec_push(tokens, new_token_eof(p));
}

enum {
    ND_NUM = 256,
};

typedef struct Node {
    int ty;
    struct Node *lhs;
    struct Node *rhs;
    int val;
} Node;

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

void gen(Node *node) {
    if (node->ty == ND_NUM) {
        printf("    push %d\n", node->val);
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
    }

    printf("    push rax\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }

    if (strcmp(argv[1], "-test") == 0) {
        runtest();
        return 0;
    }

    tokenize(argv[1]);
    Node *node = add();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    printf("    pop rax\n");
    printf("    ret\n");

    return 0;
}
