#include <stdio.h>

int main(){
    int s[50];

    char *a = (char *)&s;

    int *b = (int *) (a+4);

    int *c = ((int *) a) + 4;
    printf("%x", b);
    return 0;
}