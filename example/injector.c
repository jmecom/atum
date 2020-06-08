#include <atum.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <pid> <lib>\n", argv[0]);
        return 1;
    }

    pid_t pid = atoi(argv[1]);
    char *lib = argv[2];

    printf("Injecting %s into process %d...\n", lib, pid);

    return inject_library(pid, lib);
}
