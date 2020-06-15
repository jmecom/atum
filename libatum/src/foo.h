#include <mach/mach.h>

int load_library_into_task(task_t task, const char *library_path, void **return_value);