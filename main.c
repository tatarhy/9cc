#include "9cc.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *user_input;

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Invalid arguments\n");
    return 1;
  }

  if (strcmp(argv[1], "-test") == 0) {
    runtest();
    return 0;
  }

  user_input = argv[1];
  tokenize();
  program();
  gen_amd64();

  return 0;
}
