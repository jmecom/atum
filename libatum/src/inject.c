#include <atum.h>

#include <mach/mach.h>
#include <mach/error.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "log.h"
#include "loader.h"
#include "util.h"
#include "thread.h"

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

#define CHECK(ret, expected, function) \
    do { \
        if (ret != expected) { \
            ERROR("%s failed on line %d: (\'%s\')", function, __LINE__-1, mach_error_string(kr)); \
            return ATUM_MACH_FAILURE; \
        } \
    } while (0)

static int remote_alloc(task_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags)
{
    kern_return_t kr = mach_vm_allocate(target, address, size, VM_FLAGS_ANYWHERE);
    CHECK(kr, KERN_SUCCESS, "mach_vm_allocate");
    return ATUM_SUCCESS;
}

int inject_library(pid_t target_pid, const char *lib)
{
    if (!lib) {
        return ATUM_FAILURE;
    }

    task_t target = MACH_PORT_NULL;
    mach_vm_address_t code = 0;
    mach_vm_address_t stack = 0;
    struct stat statbuf;

    int ar = ATUM_FAILURE;
    mach_error_t kr;

    // 0. Check if target is 64bit and that library is present
    if (!process_is_64_bit(target_pid)) {
        ERROR("Target process (%d) is not 64bit", target_pid);
        return ATUM_FAILURE;
    }

    if (stat(lib, &statbuf) != 0) {
        ERROR("Cannot open library %s", lib);
        return ATUM_FAILURE;
    }

    // Acquire target task port
    kr = task_for_pid(mach_task_self(), target_pid, &target);
    CHECK(kr, KERN_SUCCESS, "task_for_pid");

    // Allocate remote memory
    ar = remote_alloc(target, &stack, STACK_SIZE, VM_FLAGS_ANYWHERE);
    CHECK(ar, ATUM_SUCCESS, "remote_alloc");

    ar = remote_alloc(target, &code, CODE_SIZE, VM_FLAGS_ANYWHERE);
    CHECK(ar, ATUM_SUCCESS, "remote_alloc");

    // Patch code
    char *possible_patch_location = loader_code;
    for (int i = 0; i < CODE_SIZE; i++) {
        possible_patch_location++;

        uint64_t addr_pthreadcreate = (uint64_t) dlsym(RTLD_DEFAULT, "pthread_create_from_mach_thread");
        uint64_t addr_dlopen = (uint64_t) dlopen;

        if (memcmp(possible_patch_location, "PTHRDCRT", 8) == 0) {
            memcpy(possible_patch_location, &addr_pthreadcreate, 8);
        }

        if (memcmp(possible_patch_location, "DLOPEN__", 6) == 0) {
            memcpy(possible_patch_location, &addr_dlopen, sizeof(uint64_t));
        }

        if (memcmp(possible_patch_location, "LIBLIBLIB", 9) == 0) {
            strcpy(possible_patch_location, lib);
        }
    }

    // Write the code
    kr = mach_vm_write(target, code, (vm_address_t) loader_code, sizeof(loader_code));
    CHECK(kr, KERN_SUCCESS, "mach_vm_write");

    // Mark code executable
    kr = vm_protect(target, code, sizeof(loader_code), FALSE, VM_PROT_READ | VM_PROT_EXECUTE);
    CHECK(kr, KERN_SUCCESS, "vm_protect");

    // Mark stack rw
    kr = vm_protect(target, stack, STACK_SIZE, TRUE, VM_PROT_READ | VM_PROT_WRITE);
    CHECK(kr, KERN_SUCCESS, "vm_protect");

    // 5. Create thread
    thread_act_t thread;
    ar = create_remote_thread(target, &thread, stack, STACK_SIZE, code, CODE_SIZE);
    CHECK(ar, ATUM_SUCCESS, "create_remote_thread");

    return ATUM_SUCCESS;
}
