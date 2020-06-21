#include <stdio.h>
#include <unistd.h>

void __attribute__((constructor)) hello()
{
    for (int i = 0 ; i < 4; i++) {
        puts("hello");
        sleep(2);
    }
}
