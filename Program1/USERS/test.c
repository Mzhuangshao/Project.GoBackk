#include <stdio.h>

void main()
{
    int milliseconds = 1348;
    printf("milliseconds = %d\n", milliseconds);
    printf("%d\n", milliseconds % 10);
    printf("%d\n", (milliseconds / 10) % 10);
    printf("%d\n", (milliseconds / 100) % 10);
    printf("%d\n", (milliseconds / 1000) % 10);
}