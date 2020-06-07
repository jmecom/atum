#include "util.h"

#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach.h>

// From https://github.com/rodionovd/RDInjectionWizard/blob/master/injector/rd_inject_library/rd_inject_library.c
bool process_is_64_bit(pid_t proc)
{
    int mib[4] = {
        CTL_KERN, KERN_PROC, KERN_PROC_PID, proc
    };
    struct kinfo_proc info;
    size_t size = sizeof(info);

    if (sysctl(mib, 4, &info, &size, NULL, 0) == KERN_SUCCESS) {
        return (info.kp_proc.p_flag & P_LP64);
    } else {
        return false;
    }
}
