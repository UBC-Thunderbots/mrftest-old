#include <errno.h>
#include <newlib.h>
#include <stdint.h>
#include <unistd.h>

#ifndef _WANT_IO_LONG_LONG
#error You must build newlib with --enable-newlib-io-long-long
#endif

extern char linker_bss_vma_end;
extern char linker_heap_end;

void *sbrk(intptr_t increment) {
	static char *break_pointer = &linker_bss_vma_end;
	char *old_break = break_pointer;
	char *new_break = break_pointer + increment;
	if (new_break > &linker_heap_end) {
		errno = ENOMEM;
		return (void *) -1;
	} else {
		break_pointer = new_break;
		return old_break;
	}
}

