#include <stdio.h>
 
int add(int a, int b);
int multiply(int a, int b);
 
int main() {
    int x = 4, y = 5;
    printf("Sum: %d\n", add(x, y));
    printf("Product: %d\n", multiply(x, y));
    return 0;
}
