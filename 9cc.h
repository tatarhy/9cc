typedef enum {
  TK_NUM = 256,
  TK_IDENT,
  TK_EQ,    // ==
  TK_NE,    // !=
  TK_LE,    // <=
  TK_GE,    // >=
  TK_INT,   // int
  TK_IF,    // if
  TK_ELSE,  // else
  TK_WHILE, // while
  TK_RET,   // return
  TK_EOF,
} TokenKind;

typedef struct Token {
  struct Token *next;
  TokenKind ty;
  int val;
  char *str;
  int len;
} Token;

enum {
  ND_NUM = 256,
  ND_IDENT,
  ND_EQ,
  ND_NE,
  ND_LE,
  ND_GE,
  ND_IF,
  ND_WHILE,
  ND_RET,
  ND_BLOCK,
  ND_CALL,
  ND_ADDR,
  ND_DEREF,
};

typedef struct {
  void **data;
  int capacity;
  int len;
} Vector;

typedef struct {
  Vector *keys;
  Vector *vals;
} Map;

typedef struct Node {
  int ty;
  struct Node *lhs;
  struct Node *rhs;

  struct Node *cond;
  struct Node *then;
  struct Node *els;
  struct Node *init;
  struct Node *inc;
  struct Node *body;

  int val;
  char *name;
  int offset;
  Vector *args;
  Vector *stmts;
} Node;

typedef struct Type {
  enum {INT, PTR} ty;
  struct Type *ptrto;
} Type;

typedef struct LVar {
    struct LVar *next;
    char *name;
    int len;
    int offset;
} LVar;

typedef struct Function {
  struct Function *next;
  char *name;
  LVar *lvar;
  int lval_len;
  int arg_len;
  Vector *code;
} Function;

LVar *find_lvar(LVar *lvar, Token *tok);

Vector *new_vector();
void vec_push(Vector *vec, void *elem);

Map *new_map();
void map_put(Map *map, char *key, int len, void *val);
void *map_get(Map *map, char *key, int len);

void runtest();

void error(char *msg, ...);
void error_at(char *msg, char *loc);

extern char *user_input;
extern Token *tokens;
extern Vector *code;
extern Function *funcs;

void tokenize();
void program();
void gen_amd64();
