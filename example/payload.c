#include <stdio.h>

void __attribute__((constructor)) hello()
{
    puts("hello");
}
