#pragma once

#include <unistd.h>
#include <mach/mach.h>

typedef enum {
    ATUM_SUCCESS = 0,
    ATUM_FAILURE = 1,         // Generic error
    ATUM_MACH_FAILURE = 3,    // Generic mach syscall failure
    ATUM_NO_PERMISSIONS = 4,  // Generic
} atum_err_t;

int inject_library(pid_t target, const char *lib);