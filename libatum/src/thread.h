#pragma once

#include <mach/mach.h>

int create_remote_thread(task_t target, thread_act_t *thread_out,
                         mach_vm_address_t stack, size_t stack_size,
                         mach_vm_address_t code, size_t code_size);

int terminate_remote_thread(task_t target, thread_act_t thread);

int register_exception_port_for_thread(thread_act_t thread);
