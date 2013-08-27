#include "flash.h"
#include "syscalls.h"
#include <string.h>

#define FLASH_SIZE (16UL / 8UL * 1024UL * 1024UL)
#define SECTOR_SIZE 4096UL
#define PARAMETERS_OFFSET (FLASH_SIZE - SECTOR_SIZE)
#define PARAMETERS_ADDRESS (0x60000000 + PARAMETERS_OFFSET)

const volatile uint8_t * const flash_params_block = (const volatile uint8_t *) PARAMETERS_ADDRESS;

/**
 * \brief The possible Flash virtual machine operations.
 */
typedef enum {
	/**
	 * \brief Marks the end of a VM program.
	 *
	 * This opcode takes no parameters.
	 */
	FLASH_OP_END = 0,

	/**
	 * \brief Writes a value to the control and status register.
	 *
	 * This opcode takes one parameter byte, the value to write.
	 */
	FLASH_OP_WCSR = 1,

	/**
	 * \brief Transceives one or more bytes, with the bytes to send being stored as literals in the VM code.
	 *
	 * This opcode takes a variable number of parameter bytes, the first being the number of bytes to send and the following being those bytes.
	 */
	FLASH_OP_XC_LIT = 2,

	/**
	 * \brief Transceives one or more bytes, with the bytes to send being stored in a separate buffer.
	 *
	 * This opcode takes five parameter bytes, the first being the number of bytes to send and the next four being a pointer to the buffer.
	 */
	FLASH_OP_XC_BUF = 3,

	/**
	 * \brief Conditionally branches to a different address.
	 *
	 * This opcode takes six parameter bytes, the first being the value to AND with the most recently received byte, the second being the value to XOR with the result, and the remaining four being the address to jump to if the result of the AND followed by the XOR is nonzero.
	 */
	FLASH_OP_BRANCH = 4,
} vm_op_t;

static uint8_t vm_buffer[] = {
	// Exit continuous mode
	/* 0 */ FLASH_OP_WCSR, 3,
	/* 2 */ FLASH_OP_XC_LIT, 4, 0xFF, 0xFF, 0xFF, 0xFF,
	/* 8 */ FLASH_OP_WCSR, 2,

	// Write enable
	/* 10 */ FLASH_OP_WCSR, 3,
	/* 12 */ FLASH_OP_XC_LIT, 1, 0x06,
	/* 15 */ FLASH_OP_WCSR, 2,

	// Sector erase
	/* 17 */ FLASH_OP_WCSR, 3,
	/* 19 */ FLASH_OP_XC_LIT, 4, 0x20, PARAMETERS_OFFSET >> 16, (uint8_t) (PARAMETERS_OFFSET >> 8), (uint8_t) PARAMETERS_OFFSET,
	/* 25 */ FLASH_OP_WCSR, 2,

	// Read status register loop
	/* 27 */ FLASH_OP_WCSR, 3,
	/* 29 */ FLASH_OP_XC_LIT, 2, 0x05, 0x05,
	/* 33 */ FLASH_OP_BRANCH, 0x01, 0x00, 0, 0, 0, 0,
	/* 40 */ FLASH_OP_WCSR, 2,

	// Write enable
	/* 42 */ FLASH_OP_WCSR, 3,
	/* 44 */ FLASH_OP_XC_LIT, 1, 0x06,
	/* 47 */ FLASH_OP_WCSR, 2,

	// Page program
	/* 49 */ FLASH_OP_WCSR, 3,
	/* 51 */ FLASH_OP_XC_LIT, 4, 0x02, PARAMETERS_OFFSET >> 16, (uint8_t) (PARAMETERS_OFFSET >> 8), (uint8_t) PARAMETERS_OFFSET,
	/* 57 */ FLASH_OP_XC_BUF, 4, 0, 0, 0, 0,
	/* 63 */ FLASH_OP_WCSR, 2,

	// Read status register loop
	/* 65 */ FLASH_OP_WCSR, 3,
	/* 67 */ FLASH_OP_XC_LIT, 2, 0x05, 0x05,
	/* 71 */ FLASH_OP_BRANCH, 0x01, 0x00, 0, 0, 0, 0,
	/* 78 */ FLASH_OP_WCSR, 2,

	/* 80 */ FLASH_OP_END
};

void flash_write_params(const void *params, size_t len) {
	const void *ptr;

	// Point the first jump.
	ptr = &vm_buffer[29];
	memcpy(&vm_buffer[36], &ptr, sizeof(ptr));

	// Fill in the length.
	vm_buffer[58] = (uint8_t) len;

	// Fill in the data pointer.
	memcpy(&vm_buffer[59], &params, sizeof(params));

	// Point the second jump.
	ptr = &vm_buffer[67];
	memcpy(&vm_buffer[74], &ptr, sizeof(ptr));

	// Execute the operation.
	syscall_flash_vm_execute(vm_buffer);
}

