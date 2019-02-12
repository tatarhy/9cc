#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "9cc.h"


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
    program();
    gen_amd64();

    return 0;
}
