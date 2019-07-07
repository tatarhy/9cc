#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Token *tokens;

Token *new_token(char *p) {
  Token *token = malloc(sizeof(Token));
  token->ty = *p;
  token->str = p;
  token->len = 1;
  return token;
}

Token *new_token_num(char *p, char **endptr) {
  Token *token = malloc(sizeof(Token));
  token->ty = TK_NUM;
  token->str = p;
  token->val = strtol(p, endptr, 10);
  token->len = *endptr - p;
  return token;
}

Token *new_token_ty(int ty, char *p, int len) {
  Token *token = malloc(sizeof(Token));
  token->ty = ty;
  token->str = p;
  token->len = len;
  return token;
}

int iskeyword(char *p, char *keyword) {
  int n = strlen(keyword);
  return strncmp(p, keyword, n) == 0 && !isalnum(p[n]) && p[n] != '_';
}

struct {
  char *name;
  int ty;
} keywords[] = {{"else", TK_ELSE},
                {"if", TK_IF},
                {"int", TK_INT},
                {"return", TK_RET},
                {"while", TK_WHILE}};

struct {
  char *name;
  int ty;
} symbols[] = {
    {"==", TK_EQ}, {"!=", TK_NE}, {"<=", TK_LE}, {">=", TK_GE}, {NULL, 0}};

void tokenize() {
  char *p = user_input;
  Token *token = malloc(sizeof(Token));
  tokens = token;

loop:
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    for (int i = 0; keywords[i].name != NULL; i++) {
      if (iskeyword(p, keywords[i].name)) {
        int len = strlen(keywords[i].name);
        token->next = new_token_ty(keywords[i].ty, p, len);
        token = token->next;
        p += len;
        goto loop;
      }
    }

    /*
     * identifier:
     *     identifier-nondigit
     *     identifier identifier-nondigit
     *     identifier digit
     * identifier-nondigit:
     *     nondigit
     *     universal-character-name // TODO
     *     other implementation-defined characters // TODO
     */
    if (isalpha(*p) || *p == '_') {
      char *bp = p;
      while (isalnum(*p) || *p == '_') {
        p++;
      }
      token->next = new_token_ty(TK_IDENT, bp, p - bp);
      token = token->next;
      continue;
    }

    for (int i = 0; symbols[i].name != NULL; i++) {
      if (strncmp(p, symbols[i].name, 2) == 0) {
        token->next = new_token_ty(symbols[i].ty, p, 2);
        token = token->next;
        p += 2;
        goto loop;
      }
    }

    if (strchr("+-*/=!<>(){},;&", *p) != NULL) {
      token->next = new_token(p);
      token = token->next;
      p++;
      continue;
    }

    if (isdigit(*p)) {
      token->next = new_token_num(p, &p);
      token = token->next;
      continue;
    }

    error_at("Cannot tokenize", p);
  }

  tokens = tokens->next;
}
