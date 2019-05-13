#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"

Vector *tokens;

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

Token *new_token_ident(char *p, int len) {
    char *name = malloc(sizeof(char) * (len + 1));
    strncpy(name, p, len);
    name[len] = '\0';
    Token *token = malloc(sizeof(Token));
    token->ty = TK_IDENT;
    token->name = name;
    return token;
}

Token *new_token_ty(char *p, int ty) {
    Token *token = malloc(sizeof(Token));
    token->ty = ty;
    token->input = p;
    return token;
}

void tokenize(char *p) {
    tokens = new_vector();
    while (*p) {
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !isalnum(p[6]) && p[6] != '_') {
            vec_push(tokens, new_token_ty(p, TK_RET));
            p += 6;
            continue;
        }

        if (isalpha(*p)) {
            char *bp = p;
            while (isalpha(*p)) {
                p++;
            }
            if (strncmp(bp, "if", p - bp) == 0) {
                vec_push(tokens, new_token_ty(bp, TK_IF));
            } else {
                vec_push(tokens, new_token_ident(bp, p - bp));
            }
            continue;
        }

        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')' || *p == ';' || *p == ',' || *p == '{' || *p == '}') {
            vec_push(tokens, new_token(p));
            p++;
            continue;
        }

        if (*p == '=') {
            p++;
            if (*p == '=') {
                vec_push(tokens, new_token_ty(p, TK_EQ));
                p++;
                continue;
            }
            vec_push(tokens, new_token(p-1));
            continue;
        }

        if (*p == '!') {
            p++;
            if (*p == '=') {
                vec_push(tokens, new_token_ty(p, TK_NE));
                p++;
                continue;
            }
            vec_push(tokens, new_token(p-1));
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
