#include "thread.h"

#include <atum.h>
#include <mach/mach.h>

#include "log.h"
#include "util.h"

kern_return_t
mach_vm_deallocate(
	mach_port_name_t target,
	mach_vm_address_t address,
	mach_vm_size_t size);

kern_return_t
mach_port_deallocate(
	ipc_space_t	space,
	mach_port_name_t name);

static int x86_create_remote_thread(task_t target, thread_act_t *thread_out,
                                    mach_vm_address_t stack, size_t stack_size,
                                    mach_vm_address_t code, size_t code_size)
{
    x86_thread_state64_t state;

    memset(&state, 0, sizeof(state));

    state.__rip = (uint64_t) (vm_address_t) code;
    state.__rsp = (uint64_t) stack + stack_size;
    state.__rbp = (uint64_t) stack;

    kern_return_t kr = thread_create_running(target, x86_THREAD_STATE64,
        (thread_state_t) &state, x86_THREAD_STATE64_COUNT, thread_out);
    if (kr != KERN_SUCCESS) {
        ERROR("thread_create_running failed ('%s')", mach_error_string(kr));
        return ATUM_FAILURE;
    }

    return ATUM_SUCCESS;
}

static int x86_terminate_remote_thread(task_t target, thread_act_t thread)
{
    mach_msg_type_number_t thread_state_count = x86_THREAD_STATE64_COUNT;
    x86_thread_state64_t state;

    // TODO make async
    for (;;) {
	    kern_return_t kr = thread_get_state(thread, x86_THREAD_STATE64, (thread_state_t) &state, &thread_state_count);
        if (kr != KERN_SUCCESS) {
            ERROR("thread_terminate failed ('%s')", mach_error_string(kr));
            return ATUM_FAILURE;
        }

        // TODO fix
        if (state.__rax == 0xD13) {
            INFO("Terminating remote thread");
            kr = thread_terminate(thread);
            if (kr != KERN_SUCCESS) {
                ERROR("thread_terminate failed ('%s')", mach_error_string(kr));
                return ATUM_FAILURE;
            }
            mach_port_deallocate(target, thread);
            break;
        }
    }

    return ATUM_SUCCESS;
}

// TODO: Get rid of macros, update build system

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

int terminate_remote_thread(task_t target, thread_act_t thread)
{
#ifdef __x86_64__
return x86_terminate_remote_thread(target, thread);
#elif __arm64_
#error "ARM is not yet supported"
#else
#error "Unsupported platform"
#endif
}
