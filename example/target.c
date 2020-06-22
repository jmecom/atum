#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>

void count_threads(void)
{
	thread_act_array_t threads;
	mach_msg_type_number_t thread_count;

	kern_return_t kr = task_threads(mach_task_self(), &threads, &thread_count);
    if (kr != KERN_SUCCESS) {
        exit(123);
    }

    fprintf(stderr, "[target] thread count: %d\n", thread_count);

    for (int i = 0; i < thread_count; i++) {
        mach_port_deallocate(mach_task_self(), threads[i]);
    }
    vm_deallocate(mach_task_self(), (vm_address_t)threads, sizeof(thread_t) * thread_count);
}

int main(void)
{
    for (;;) {
        count_threads();
        sleep(1);
    }
}
