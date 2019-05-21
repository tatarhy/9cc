#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int iskeyword(char *p, char *keyword) {
  int n = strlen(keyword);
  return strncmp(p, keyword, n) == 0 && !isalnum(p[n]) && p[n] != '_';
}

struct {
  char *name;
  int ty;
} symbols[] = {
    {"==", TK_EQ}, {"!=", TK_NE}, {"<=", TK_LE}, {">=", TK_GE}, {NULL, 0}};

void tokenize() {
  char *p = user_input;
  tokens = new_vector();

loop:
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (iskeyword(p, "return")) {
      vec_push(tokens, new_token_ty(p, TK_RET));
      p += 6;
      continue;
    }

    if (iskeyword(p, "if")) {
      vec_push(tokens, new_token_ty(p, TK_IF));
      p += 2;
      continue;
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
      vec_push(tokens, new_token_ident(bp, p - bp));
      continue;
    }

    for (int i = 0; symbols[i].name != NULL; i++) {
      if (strncmp(p, symbols[i].name, 2) == 0) {
        vec_push(tokens, new_token_ty(p, symbols[i].ty));
        p += 2;
        goto loop;
      }
    }

    if (strchr("+-*/=!(){},;", *p) != NULL) {
      vec_push(tokens, new_token(p));
      p++;
      continue;
    }

    if (isdigit(*p)) {
      vec_push(tokens, new_token_num(p, &p));
      continue;
    }

    error_at("Cannot tokenize", p);
  }

  vec_push(tokens, new_token_ty(p, TK_EOF));
}
