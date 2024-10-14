#include "lib-1.h"
#include <stddef.h> // definition for NULL
#include <stdlib.h>

int f1(int i) {
    return i;
}

static int d[2] = {3,5};
int* fptr1(int k) {
    if (k % 2 == 0) {
        // return &d[0];
        d[0] += 1; // logical change to stop compiler optimization
        return d;
    } else {
        // return &d[1];
        d[1] += 1;
        return d + 1; // pointer arithmetic
    }
}

static int *d2 = NULL;

int* fptr2(int k) {
    if (d2 == NULL) {
        d2 = calloc(2000000, sizeof(*d2));
    }
    if (k % 2 == 0) {
        d2[0] += 1;
        return d2;
    } else {
        d2[1] += 1;
        return d2 + 1;
    }
}

#if 0
/*
int x; // 1
int *x; // -> 1, -> [1,2,3]
int **x;
int *x = {1,2,3};
*/
#endif