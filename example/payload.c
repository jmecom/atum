#include <stdio.h>
#include <unistd.h>

void __attribute__((constructor)) hello()
{
    for (;;) {
        puts("hello");
        sleep(2);
    }
}
