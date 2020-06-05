#include <stdio.h>
#include <unistd.h>
#include <dispatch/dispatch.h>

void go(void)
{
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        go();
    });
}

int main(void)
{
    go();
    dispatch_main();
}
