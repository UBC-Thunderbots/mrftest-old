static void syscalls_impl(void) __attribute__((used));
static void syscalls_impl(void) {
	asm volatile(
			".global syscall_debug_puts\n\t"
			".type syscall_debug_puts, function\n\t"
			"syscall_debug_puts:\n\t"
			"retl\n\t"
			"ta 0\n\t"
			"\n\t"
			".global syscall_flash_vm_execute\n\t"
			".type syscall_flash_vm_execute, function\n\t"
			"syscall_flash_vm_execute:\n\t"
			"retl\n\t"
			"ta 1\n\t");
}

