#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>
#include <pthread.h>
#include <mach/mach.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <mach-o/dyld.h>
#include <dispatch/dispatch.h>
#include <mach/machine/thread_status.h>

#include "foo.h"

#define kRDRemoteStackSize      (25*1024)
#define kRDShouldJumpToDlopen   0xabad1dea

kern_return_t mach_vm_allocate(
        vm_map_t target,
        mach_vm_address_t *address,
        mach_vm_size_t size,
        int flags
);

kern_return_t mach_vm_deallocate(vm_map_t target, mach_vm_address_t address, mach_vm_size_t size);

kern_return_t mach_vm_write(
        vm_map_t target_task,
        mach_vm_address_t address,
        vm_offset_t data,
        mach_msg_type_number_t dataCnt
);

/* Received from xnu/bsd/uxkern/ux_exception.c
 * (Apple's XNU version 2422.90.20)
 */
typedef struct {
    mach_msg_header_t Head;
    /* start of the kernel processed data */
    mach_msg_body_t msgh_body;
    mach_msg_port_descriptor_t thread;
    mach_msg_port_descriptor_t task;
    /* end of the kernel processed data */
    NDR_record_t NDR;
    exception_type_t exception;
    mach_msg_type_number_t codeCnt;
    mach_exception_data_t code;
    /* some times RCV_TO_LARGE probs */
    char pad[512];
} exc_msg_t;

#define RDFailOnError(function) {if (err != KERN_SUCCESS) {printf("[%d] %s failed with error: %s\n", \
    __LINE__-1, function, mach_error_string(err)); return (err);}}



mach_port_t init_exception_port_for_thread(thread_act_t thread, thread_state_flavor_t thread_flavor)
{
    int err = KERN_FAILURE;
    /* Allocate an exception port */
    mach_port_name_t exception_port;
    err = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &exception_port);
    if (err != KERN_SUCCESS) return (0);
    err = mach_port_insert_right(mach_task_self(), exception_port, exception_port,
                                 MACH_MSG_TYPE_MAKE_SEND);
    if (err != KERN_SUCCESS) return (0);
    /* Assign this port to our thread */
    err = thread_set_exception_ports(thread, EXC_MASK_ALL,
                                     exception_port, EXCEPTION_STATE_IDENTITY,
                                     thread_flavor);
    if (err != KERN_SUCCESS) return (0);

    return exception_port;
}

int load_library_into_task(task_t task, const char *library_path, void **return_value)
{
    if (!task) return KERN_INVALID_ARGUMENT;
    int err = KERN_FAILURE;

    /* Copy the library path into target's address space */
    size_t path_size = strlen(library_path) + 1;
    mach_vm_address_t rlibrary = 0;
    err = mach_vm_allocate(task, &rlibrary, path_size, VM_FLAGS_ANYWHERE);
    RDFailOnError("mach_vm_allocate");
    err = mach_vm_write(task, rlibrary, (vm_offset_t)library_path,
                        (mach_msg_type_number_t)path_size);
    RDFailOnError("mach_vm_write");

    /* Compose a fake backtrace and allocate remote stack. */
    uint64_t fake_backtrace[] = {
        kRDShouldJumpToDlopen
    };
    mach_vm_address_t stack = 0;
    err = mach_vm_allocate(task, &stack, (kRDRemoteStackSize + sizeof(fake_backtrace)),
                           VM_FLAGS_ANYWHERE);
    RDFailOnError("mach_vm_allocate");

    /* Reserve some place for a pthread struct */
    mach_vm_address_t pthread_struct = stack;

    /* Copy the backtrace to the top of remote stack */
    err = mach_vm_write(task, (stack + kRDRemoteStackSize), (vm_offset_t)fake_backtrace,
                        (mach_msg_type_number_t)sizeof(fake_backtrace));
    RDFailOnError("mach_vm_write");

    /* Initialize a remote thread state */
    x86_thread_state64_t state;
    memset(&state, 0, sizeof(state));
    state.__rbp = stack;
    state.__rsp = stack + kRDRemoteStackSize;

    printf("FOO 1\n");

    /* We'll jump right into pthread_create_from_mach_thread() to convert
     * our mach thread into real POSIX thread */
    void *pthread_create_from_mach_thread = dlsym(RTLD_DEFAULT, "pthread_create_from_mach_thread");
    if (!pthread_create_from_mach_thread) {
        err = KERN_INVALID_HOST;
        RDFailOnError("dlsym(\"_pthread_create_from_mach_thread\")");
    }

    // TODO: Need to set this up correctly?

    state.__rip = (mach_vm_address_t)pthread_create_from_mach_thread;
    state.__rdi = pthread_struct;
    state.__rsi = 0;
    // state.__rdx = 0x1337; // THIS IS THE START ROUTINE FOR THE THREAD.
    printf("addr of dlopen %p\n", &dlopen);
    state.__rdx = (uint64_t)&dlopen;
    state.__rcx = 0;

    /* Store the library path here for dlopen() */
    state.__rbx = rlibrary;

    printf("FOO 2\n");

    /* Create a remote thread, set up an exception port for it
     * (so we will be able to handle ECX_BAD_ACCESS exceptions) */
    thread_act_t remote_thread = 0;
    err = thread_create(task, &remote_thread);
    RDFailOnError("thead_create");
    err = thread_set_state(remote_thread, x86_THREAD_STATE64,
                           (thread_state_t)&state, x86_THREAD_STATE64_COUNT);
    RDFailOnError("thread_set_state");

    printf("FOO 3\n");

    mach_port_t exception_port = 0;
    exception_port = init_exception_port_for_thread(remote_thread, x86_THREAD_STATE64);
    if (!exception_port) {
        err = KERN_FAILURE;
        RDFailOnError("init_exception_handler_for_thread");
    }
    err = thread_resume(remote_thread);
    RDFailOnError("thread_resume");

    printf("FOO 4\n");

    /* Exception handling loop */
    while (1) {
        printf("FOO 5\n");
        /* Use exc_server() as a generic server for sending/receiving exception messages */
        extern boolean_t exc_server(mach_msg_header_t *request, mach_msg_header_t *reply);
        err = mach_msg_server_once(exc_server, sizeof(exc_msg_t), exception_port, 0);
        RDFailOnError("mach_msg_server_once");
        printf("FOO 6\n");

        thread_basic_info_data_t thread_basic_info;
        mach_msg_type_number_t thread_basic_info_count = THREAD_BASIC_INFO_COUNT;
        err = thread_info(remote_thread, THREAD_BASIC_INFO,
                          (thread_info_t)&thread_basic_info, &thread_basic_info_count);
        RDFailOnError("thread_info");

        /* Chech if we've already suspended the thread inside our exception handler */
        if (thread_basic_info.suspend_count > 0) {
            /* So retrive the dlopen() return value first */
            if (return_value) {
                printf("FOO 7\n");
                x86_thread_state64_t updated_state;
                mach_msg_type_number_t state_count = x86_THREAD_STATE64_COUNT;
                err = thread_get_state(remote_thread, x86_THREAD_STATE64,
                                       (thread_state_t)&updated_state, &state_count);
                RDFailOnError("thread_get_state");
                *return_value = (void *)updated_state.__rax;
            }

            /* then terminate the remote thread */
            err = thread_terminate(remote_thread);
            RDFailOnError("thead_terminate");
            /* and do some memory clean-up */
            err = mach_vm_deallocate(task, rlibrary, path_size);
            RDFailOnError("mach_vm_deallocate");
            err = mach_vm_deallocate(task, stack, kRDRemoteStackSize);
            RDFailOnError("mach_vm_deallocate");
            err = mach_port_deallocate(mach_task_self(), exception_port);
            RDFailOnError("mach_port_deallocate");

            printf("FOO 8\n");
            break;
        }
    }

    return err;
}



#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
/**
 * @abstract
 * Custom EXC_BAD_ACCESS exception handler, called by exc_server().
 *
 * @return
 * KERN_SUCCESS indicates that we've reset a thread state and it's ready to be run again
 * @return
 * MIG_NO_REPLY indicates that we've terminated or suspended the thread, so the kernel won't
 *              handle it anymore. This one also breaks our exception handling loop, so we
 *              can finish the injection process
 * @return
 * KERN_FAILURE indicates that we was not expect this kind of exception and the kernel should
 *              find another handler for it
 *
 */
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

    printf("FOO 8.5\n");
    if (*flavor != x86_THREAD_STATE64) {
        return KERN_FAILURE;
    }

    if (((x86_thread_state64_t *)in_state)->__rip == kRDShouldJumpToDlopen) {
        /* Prepare the thread to execute dlopen() */
        printf("addr of dlopen %p\n", &dlopen);
        ((x86_thread_state64_t *)out_state)->__rip = (uint64_t)&dlopen;
        ((x86_thread_state64_t *)out_state)->__rsi = RTLD_NOW | RTLD_LOCAL;
        ((x86_thread_state64_t *)out_state)->__rdi = ((x86_thread_state64_t *)in_state)->__rbx;
        /* Preserve the stack pointer */
        uint64_t stack = ((x86_thread_state64_t *)in_state)->__rsp;
        ((x86_thread_state64_t *)out_state)->__rsp =  stack;
        /* Indicate that we've updateed this thread state and ready to resume it */
        *out_state_count = x86_THREAD_STATE64_COUNT;

        printf("FOO 9\n");

        return KERN_SUCCESS;
    }
    /* In any other case we want to gracefully terminate the thread, so our
     * target won't crash. */
    thread_suspend(thread);

    return MIG_NO_REPLY;
}
#pragma clang diagnostic pop // ignored "-Wmissing-prototypes"