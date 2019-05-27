enum {
  TK_NUM = 256,
  TK_IDENT,
  TK_EQ,    // ==
  TK_NE,    // !=
  TK_LE,    // <=
  TK_GE,    // >=
  TK_INT,   // int
  TK_IF,    // if
  TK_WHILE, // while
  TK_RET,   // return
  TK_EOF,
};

typedef struct {
  int ty;
  int val;
  char *name;
  char *input;
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
  int val;
  char *name;
  Vector *args;
  Vector *stmts;
} Node;

typedef struct Type {
  enum {INT, PTR} ty;
  struct Type *ptrto;
} Type;

typedef struct {
  char *name;
  Map *lval;
  int lval_len;
  int arg_len;
  Vector *code;
} Function;

Vector *new_vector();
void vec_push(Vector *vec, void *elem);

Map *new_map();
void map_put(Map *map, char *key, void *val);
void *map_get(Map *map, char *key);

void runtest();

void error(char *msg, ...);
void error_at(char *msg, char *loc);

extern char *user_input;
extern Vector *tokens;
extern Vector *code;
extern Vector *funcs;

void tokenize();
void program();
void gen_amd64();
