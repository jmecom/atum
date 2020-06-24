#pragma once

#ifdef __arm64__
#error "No ARM support yet"
#else
char trampoline[] =
    // "\xCC"                            // int3

    "\x55"                            // push       rbp
    "\x48\x89\xE5"                    // mov        rbp, rsp
    "\x48\x83\xEC\x10"                // sub        rsp, 0x10
    "\x48\x8D\x7D\xF8"                // lea        rdi, qword [rbp+var_8]
    "\x31\xC0"                        // xor        eax, eax
    "\x89\xC1"                        // mov        ecx, eax
    "\x48\x8D\x15\x21\x00\x00\x00"    // lea        rdx, qword ptr [rip + 0x21]
    "\x48\x89\xCE"                    // mov        rsi, rcx
    "\x48\xB8"                        // movabs     rax, pthread_create_from_mach_thread
    "PTHRDCRT"
    "\xFF\xD0"                        // call       rax
    "\x89\x45\xF4"                    // mov        dword [rbp+var_C], eax
    "\x48\x83\xC4\x10"                // add        rsp, 0x10
    "\x5D"                            // pop        rbp
    "\x48\xc7\xc0\x13\x0d\x00\x00"    // mov        rax, 0xD13
    // "\xEB\xFE"                        // jmp        0x0
    "\xC3";                            // ret

// char call_dlopen[] =
    // "\x55"                            // push       rbp
    // "\x48\x89\xE5"                    // mov        rbp, rsp
    // "\x48\x83\xEC\x10"                // sub        rsp, 0x10
    // "\xBE\x01\x00\x00\x00"            // mov        esi, 0x1
    // "\x48\x89\x7D\xF8"                // mov        qword [rbp+var_8], rdi
    // "\x48\x8D\x3D\x1D\x00\x00\x00"    // lea        rdi, qword ptr [rip + 0x2c]
    // "\x48\xB8"                        // movabs     rax, dlopen
    // "DLOPEN__"
    // "\xFF\xD0"                        // call       rax
    // "\x31\xF6"                        // xor        esi, esi
    // "\x89\xF7"                        // mov        edi, esi
    // "\x48\x89\x45\xF0"                // mov        qword [rbp+var_10], rax
    // "\x48\x89\xF8"                    // mov        rax, rdi
    // "\x48\x83\xC4\x10"                // add        rsp, 0x10
    // "\x5D"                            // pop        rbp
    // "\xC3"                            // ret

    // "LIBLIBLIBLIB"
    // "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    // "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    // "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    // "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
#endif
