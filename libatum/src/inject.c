#include <atum.h>

#include <mach/mach.h>
#include <mach/error.h>

#include "log.h"
#include "loader.h"
#include "util.h"

kern_return_t mach_vm_allocate(
        vm_map_t target,
        mach_vm_address_t *address,
        mach_vm_size_t size,
        int flags
);

kern_return_t mach_vm_write(
        vm_map_t target_task,
        mach_vm_address_t address,
        vm_offset_t data,
        mach_msg_type_number_t dataCnt
);

#define STACK_SIZE (65536)
#define CODE_SIZE  (128)

static int remote_alloc(task_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags)
{
    kern_return_t kr;
    if ((kr = mach_vm_allocate(target, address, size, VM_FLAGS_ANYWHERE)) != KERN_SUCCESS) {
        ERROR("Failed to allocate memory ('%s')", mach_error_string(kr));
        return ATUM_MACH_FAILURE;
    }
    return ATUM_SUCCESS;
}

int inject_library(pid_t target_pid, const char *lib)
{
    if (!lib) {
        return ATUM_FAILURE;
    }

    task_t target = MACH_PORT_NULL;
    mach_vm_address_t text = 0;
    mach_vm_address_t stack = 0;

    int ar = ATUM_FAILURE;
    mach_error_t kr;

    // 0. Check if target is 64bit
    if (!process_is_64_bit(target_pid)) {
        ERROR("Target process (%d) is not 64bit", target_pid);
        return ATUM_FAILURE;
    }

    // 1. Acquire target task port
    kr = task_for_pid(mach_task_self(), target_pid, &target);
    if (kr != KERN_SUCCESS) {
        ERROR("task_for_pid on %d failed ('%s')", target_pid, mach_error_string(kr));
        return ATUM_MACH_FAILURE;
    }

    // 2. Allocate remote memory
    ar = remote_alloc(target, &stack, STACK_SIZE, VM_FLAGS_ANYWHERE);
    if (ar != ATUM_SUCCESS) {
        return ar;
    }

    ar = remote_alloc(target, &text, CODE_SIZE, VM_FLAGS_ANYWHERE);
    if (ar != ATUM_SUCCESS) {
        return ar;
    }

    // 3. Patch code

    // [todo]

    // 4. Write the code
    kr = mach_vm_write(target, text, (vm_address_t) loader_code, sizeof(loader_code));
    if (kr != KERN_SUCCESS) {
        ERROR("Unable to write code ('%s')", mach_error_string(kr));
        return ATUM_MACH_FAILURE;
    }

    // Mark code executable
    kr = vm_protect(target, text, sizeof(loader_code), FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    if (kr != KERN_SUCCESS) {
        ERROR("Couldn't mark code as executable ('%s')", mach_error_string(kr));
        return ATUM_MACH_FAILURE;
    }

    // Mark stack readble
    kr = vm_protect(target, stack, STACK_SIZE, TRUE, VM_PROT_READ);
    if (kr != KERN_SUCCESS) {
        ERROR("Couldn't mark stack as readable ('%s')", mach_error_string(kr));
        return ATUM_MACH_FAILURE;
    }

    // 5. Create thread

    return ATUM_SUCCESS;
}
