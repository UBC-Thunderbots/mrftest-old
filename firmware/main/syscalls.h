#ifndef SYSCALLS_H
#define SYSCALLS_H

#include <stdint.h>

void syscall_debug_puts(const char *message);
uint32_t syscall_flash_vm_execute(const void *vmops);

#endif

