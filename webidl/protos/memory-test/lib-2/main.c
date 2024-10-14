#include <stdio.h>
#include "lib-1.h"

int f2(int i) {
    return 2 * i;
}

int f3(int *i) {
    return 3 * *i;
}

int main(int argc, char**argv) {
    int x = f2(2);
    printf("%d\n", x); // 4
    int y = f3(&x); 
    printf("%d\n", y); // 12
    int a = f1(0);
    printf("%d\n", a); // 0
    int* bstatic = fptr1(0);
    printf("%d %p\n", *bstatic, bstatic); // 4 and stack address
    int* bheap = fptr2(0);
    printf("%d %p\n", *bheap, bheap); // 1 and heap address

    bstatic = fptr1(0);
    printf("%d %p\n", *bstatic, bstatic); // 5 and stack address
    bheap = fptr2(0);
    printf("%d %p\n", *bheap, bheap); // 2 and heap address


}