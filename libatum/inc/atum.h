#pragma once

#include <unistd.h>

typedef enum {
    ATUM_SUCCESS = 0,
    ATUM_FAILURE = 1,
    ATUM_NO_PERMISSIONS = 2,
} atum_err_t;

int inject_library(pid_t target, const char *lib);
