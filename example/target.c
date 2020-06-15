#include <unistd.h>
#include <stdio.h>

int main(void)
{
    int i = 0;
    for (;;) {
        fprintf(stderr, "[target] %d\n", ++i);
        sleep(1);
    }
}
