#include "9cc.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Report error
 */
void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

/**
 * Report error with occured position
 */
void error_at(char *msg, char *loc) {
  int pos = loc - user_input;
  fprintf(stderr, "error: %s\n", msg);
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s^\n", pos, "");
  exit(1);
}

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

Map *new_map() {
  Map *map = malloc(sizeof(Map));
  map->keys = new_vector();
  map->vals = new_vector();
  return map;
}

void map_put(Map *map, char *key, int len, void *val) {
  char *name = malloc((len + 1) * sizeof(char));
  strncpy(name, key, len);
  name[len] = '\0';
  vec_push(map->keys, key);
  vec_push(map->vals, val);
}

void *map_get(Map *map, char *key, int len) {
  for (int i = map->keys->len - 1; i >= 0; i--) {
    if (strncmp(map->keys->data[i], key, len) == 0) {
      return map->vals->data[i];
    }
  }
  return NULL;
}

void expect(int line, int expected, int actual) {
  if (expected == actual) {
    return;
  }
  fprintf(stderr, "%d: %d expected, but got %d\n", line, expected, actual);
  exit(1);
}

void test_vector() {
  Vector *vec = new_vector();
  expect(__LINE__, 0, vec->len);

  for (int i = 0; i < 100; i++) {
    vec_push(vec, (void *)i);
  }

  expect(__LINE__, 100, vec->len);
  expect(__LINE__, 0, (int)vec->data[0]);
  expect(__LINE__, 50, (int)vec->data[50]);
  expect(__LINE__, 99, (int)vec->data[99]);
}

void test_map() {
  Map *map = new_map();
  expect(__LINE__, 0, (int)map_get(map, "foo", 3));

  map_put(map, "foo", 3, (void *)2);
  expect(__LINE__, 2, (int)map_get(map, "foo", 3));

  map_put(map, "bar", 3, (void *)4);
  expect(__LINE__, 4, (int)map_get(map, "bar", 3));

  map_put(map, "foo", 3, (void *)6);
  expect(__LINE__, 6, (int)map_get(map, "foo", 3));
}

void runtest() {
  test_vector();
  test_map();

  printf("OK\n");
}
