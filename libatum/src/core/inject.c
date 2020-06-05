#include <atum.h>

#include "log.h"

int inject_library(pid_t target, const char *lib)
{
    if (!lib) {
        return ATUM_FAILURE;
    }
    INFO("test");
    return ATUM_SUCCESS;
}
