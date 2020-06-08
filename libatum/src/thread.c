#include "thread.h"

#include <atum.h>
#include <mach/mach.h>

#include "log.h"

static int x86_create_remote_thread(task_t target, thread_act_t *thread_out,
                                    mach_vm_address_t stack, size_t stack_size,
                                    mach_vm_address_t code, size_t code_size)
{
    x86_thread_state64_t state;

    memset(&state, 0, sizeof(state));

    stack += 2048;

    state.__rip = (uint64_t) (vm_address_t) code;
    state.__rsp = (uint64_t) stack;
    state.__rbp = (uint64_t) stack;

    kern_return_t kr = thread_create_running(target, x86_THREAD_STATE64,
        (thread_state_t) &state, x86_THREAD_STATE64_COUNT, thread_out);
    if (kr != KERN_SUCCESS) {
        ERROR("Failed to create thread ('%s')", mach_error_string(kr));
        return ATUM_MACH_FAILURE;
    }

    return ATUM_SUCCESS;
}

int create_remote_thread(task_t target, thread_act_t *thread_out,
                         mach_vm_address_t stack, size_t stack_size,
                         mach_vm_address_t code, size_t code_size)
{
#ifdef __x86_64__
return x86_create_remote_thread(target, thread_out, stack, stack_size, code, code_size);
#elif __arm64_
#error "ARM is not yet supported"
#else
#error "Unsupported platform"
#endif
}
