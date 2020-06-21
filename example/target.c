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
}

int main(void)
{
    int i = 0;
    for (;;) {
        fprintf(stderr, "[target] %d\n", ++i);
        count_threads();
        sleep(1);
    }
}
