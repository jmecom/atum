#include <atum.h>

#include <mach/error.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <mach/machine/thread_status.h>
#include <stdlib.h>
#include <limits.h>

#include "log.h"
#include "trampoline.h"
#include "util.h"
#include "thread.h"
#include "mach_priv.h"

#define STACK_SIZE (65536)
#define CODE_SIZE  (128)

#define MAGIC_RETURN (0xc0ffeeee)

#define MACH_CHECK(function) \
    do { \
        if (kr != KERN_SUCCESS) { \
            ERROR("%s failed @ %s:%d (\'%s\')", function, __FILE__, __LINE__-1, mach_error_string(kr)); \
            return ATUM_MACH_FAILURE; \
        } \
    } while (0)

#define ATUM_CHECK(function) \
    do { \
        if (ar != ATUM_SUCCESS) { \
            ERROR("%s failed @ %s:%d", function, __FILE__, __LINE__-1); \
            return ATUM_FAILURE; \
        } \
    } while (0)

static mach_vm_address_t g_lib_addr = 0;

static int remote_alloc(task_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags)
{
    kern_return_t kr = mach_vm_allocate(target, address, size, VM_FLAGS_ANYWHERE);
    MACH_CHECK("mach_vm_allocate");
    return ATUM_SUCCESS;
}

int inject_library(pid_t target_pid, const char *lib_path)
{
    if (!lib_path) {
        return ATUM_FAILURE;
    }

    task_t target = MACH_PORT_NULL;
    mach_vm_address_t code = 0;
    mach_vm_address_t stack = 0;
    char lib[PATH_MAX] = {0};

    int ar = ATUM_FAILURE;
    mach_error_t kr;

    // Check if target is 64bit and that library is present
    if (!process_is_64_bit(target_pid)) {
        ERROR("Target process (%d) is not 64bit", target_pid);
        return ATUM_FAILURE;
    }

    if (realpath(lib_path, lib) != 0) {
        ERROR("Cannot resolve library %s", lib_path);
        return ATUM_FAILURE;
    }

    // Acquire target task port
    kr = task_for_pid(mach_task_self(), target_pid, &target);
    MACH_CHECK("task_for_pid");

    // Allocate remote memory

    // Allocate the library path in target
    size_t path_size = strlen(lib) + 1;
    kr = mach_vm_allocate(target, &g_lib_addr, path_size, VM_FLAGS_ANYWHERE);
    MACH_CHECK("mach_vm_allocate");
    kr = mach_vm_write(target, g_lib_addr, (vm_offset_t) lib, (mach_msg_type_number_t) path_size);
    MACH_CHECK("mach_vm_write");

    // Set up the stack so that we return to our magic value
    uint64_t backtrace[] = {MAGIC_RETURN};
    ar = remote_alloc(target, &stack, STACK_SIZE + sizeof(backtrace), VM_FLAGS_ANYWHERE);
    ATUM_CHECK("remote_alloc");

    kr = mach_vm_write(target, stack + STACK_SIZE, (vm_offset_t) backtrace, (mach_msg_type_number_t) sizeof(backtrace));
    MACH_CHECK("mach_vm_write");

    ar = remote_alloc(target, &code, CODE_SIZE, VM_FLAGS_ANYWHERE);
    ATUM_CHECK("remote_alloc");

    // Patch trampoline
    char *possible_patch_location = trampoline;
    for (int i = 0; i < CODE_SIZE; i++) {
        possible_patch_location++;

        uint64_t addr_pthreadcreate = (uint64_t) dlsym(RTLD_DEFAULT, "pthread_create_from_mach_thread");
        // uint64_t addr_dlopen = (uint64_t) dlopen;

        if (memcmp(possible_patch_location, "PTHRDCRT", 8) == 0) {
            memcpy(possible_patch_location, &addr_pthreadcreate, 8);
        }

        // if (memcmp(possible_patch_location, "DLOPEN__", 6) == 0) {
        //     memcpy(possible_patch_location, &addr_dlopen, sizeof(uint64_t));
        // }

        // if (memcmp(possible_patch_location, "LIBLIBLIB", 9) == 0) {
        //     strcpy(possible_patch_location, lib);
        // }
    }

    // Write the trampoline
    kr = mach_vm_write(target, code, (vm_address_t) trampoline, sizeof(trampoline));
    MACH_CHECK("mach_vm_write");

    // Mark code executable
    kr = vm_protect(target, code, sizeof(trampoline), FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    MACH_CHECK("vm_protect");

    // Mark stack rw
    kr = vm_protect(target, stack, STACK_SIZE, TRUE, VM_PROT_READ | VM_PROT_WRITE);
    MACH_CHECK("vm_protect");

    // Create thread
    thread_act_t thread;
    ar = create_remote_thread(target, &thread, stack, STACK_SIZE, code, CODE_SIZE);
    ATUM_CHECK("create_remote_thread");

    // Register exception port and run thread
    mach_port_t exception_port = 0;
    exception_port = register_exception_port_for_thread(thread);
    if (exception_port < 0) {
        return ATUM_FAILURE;
    }
    kr = thread_resume(thread);
    MACH_CHECK("thread_resume");

    INFO("About to loop");

    // TODO: Move code
    uint64_t return_value = 0xf00dface;
    for (;;) {
        extern boolean_t exc_server(mach_msg_header_t *request, mach_msg_header_t *reply);
        kr = mach_msg_server_once(exc_server, sizeof(exc_msg_t), exception_port, 0);
        MACH_CHECK("mach_msg_server_once");

        thread_basic_info_data_t thread_basic_info;
        mach_msg_type_number_t thread_basic_info_count = THREAD_BASIC_INFO_COUNT;
        kr = thread_info(thread, THREAD_BASIC_INFO, (thread_info_t)&thread_basic_info, &thread_basic_info_count);
        MACH_CHECK("thread_info");

        /* Chech if we've already suspended the thread inside our exception handler */
        if (thread_basic_info.suspend_count > 0) {
            /* So retrive the dlopen() return value first */
            if (return_value != 0xf00dface) {
                x86_thread_state64_t updated_state;
                mach_msg_type_number_t state_count = x86_THREAD_STATE64_COUNT;
                kr = thread_get_state(thread, x86_THREAD_STATE64,
                                       (thread_state_t)&updated_state, &state_count);
                MACH_CHECK("thread_get_state");
                return_value = (uint64_t)updated_state.__rax;
            }

            /* then terminate the remote thread */
            kr = thread_terminate(thread);
            MACH_CHECK("thead_terminate");
            /* and do some memory clean-up */
            kr = mach_vm_deallocate(target, g_lib_addr, path_size);
            MACH_CHECK("mach_vm_deallocate");
            kr = mach_vm_deallocate(target, stack, STACK_SIZE);
            MACH_CHECK("mach_vm_deallocate");
            kr = mach_port_deallocate(mach_task_self(), exception_port);
            MACH_CHECK("mach_port_deallocate");
            break;
        }
    }

    // Terminate thread
    // ar = terminate_thread(target, thread);
    // ATUM_CHECK("terminate_thread");

    // TOOD: Deallocate?

    return ATUM_SUCCESS;
}

// TODO: It's x86 specific
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
__attribute__((visibility("default")))
kern_return_t
catch_exception_raise_state_identity(mach_port_t exception_port, mach_port_t thread,
                                     mach_port_t task, exception_type_t exception,
                                     exception_data_t code, mach_msg_type_number_t code_count,
                                     int *flavor, thread_state_t in_state,
                                     mach_msg_type_number_t in_state_count,
                                     thread_state_t out_state,
                                     mach_msg_type_number_t *out_state_count)
{
#pragma unused (exception_port, task)
#pragma unused (exception, code, code_count, in_state_count, out_state, out_state_count)

    if (*flavor != x86_THREAD_STATE64) {
        return KERN_FAILURE;
    }

    INFO("Inside exception handler");

    if (((x86_thread_state64_t *)in_state)->__rip == MAGIC_RETURN) {
        /* Prepare the thread to execute dlopen() */
        ((x86_thread_state64_t *)out_state)->__rip = (uint64_t) &dlopen;
        INFO("dlopen @ %p", &dlopen);
        ((x86_thread_state64_t *)out_state)->__rsi = RTLD_NOW | RTLD_LOCAL;
        // ((x86_thread_state64_t *)out_state)->__rdi = ((x86_thread_state64_t *)in_state)->__rbx;
        // ((x86_thread_state64_t *)out_state)->__rbx = g_lib_addr;
        ((x86_thread_state64_t *)out_state)->__rdi = (uint64_t) g_lib_addr;
        /* Preserve the stack pointer */
        uint64_t stack = ((x86_thread_state64_t *)in_state)->__rsp;
        ((x86_thread_state64_t *)out_state)->__rsp = stack;
        /* Indicate that we've updated this thread state and ready to resume it */
        *out_state_count = x86_THREAD_STATE64_COUNT;

        return KERN_SUCCESS;
    }
    /* In any other case we want to gracefully terminate the thread, so our
     * target won't crash. */
    thread_suspend(thread);

    return MIG_NO_REPLY;
}
#pragma clang diagnostic pop // ignored "-Wmissing-prototypes"
