enum {
    TK_NUM = 256,
    TK_IDENT,
    TK_EQ, // ==
    TK_NE, // !=
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
    ND_CALL,
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
} Node;

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

extern Vector *tokens;
extern Vector *code;
extern Vector *funcs;

void tokenize(char *p);
void program();
void gen_amd64();
