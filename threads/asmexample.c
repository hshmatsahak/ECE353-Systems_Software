
#include <stdio.h>

struct fanint{
    int x;
};

int main(int argc, char **argv){
    struct fanint a = {5};
    struct fanint b = a;
    b.x = 20;
    printf("%d", a.x);
    printf("%d", b.x);
    return 0;
}