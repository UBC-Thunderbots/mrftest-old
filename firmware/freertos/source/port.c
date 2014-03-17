#include <assert.h>
#include <FreeRTOS.h>
#include <inttypes.h>
#include <semphr.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <task.h>
#include <registers/mpu.h>
#include <registers/nvic.h>
#include <registers/systick.h>

typedef struct {
	unsigned long r0;
	unsigned long r1;
	unsigned long r2;
	unsigned long r3;
	unsigned long r12;
	unsigned long lr;
	unsigned long return_address;
	unsigned long xpsr;
} basic_hw_frame_t;

typedef struct {
	unsigned long control;
	void *stack_guard_rbar;
	MPU_RASR_t stack_guard_rasr;
	unsigned long r4;
	unsigned long r5;
	unsigned long r6;
	unsigned long r7;
	unsigned long r8;
	unsigned long r9;
	unsigned long r10;
	unsigned long r11;
	unsigned long lr;
} basic_sw_frame_t;



extern char linker_mstack_vma, linker_mstack_vma_end;

// Layout of the CCM itself.
#define CCM_BASE 0x10000000U
#define CCM_SIZE 65536U

// Size of a CCM block header, which also defines the minimum alignment requirements.
#define CCM_BLOCK_HEADER_SIZE 32U

// Layout of a CCM block header.
typedef struct {
	// Size of this block, including header.
	size_t size;
	// Size of the preceding block, including header (zero if no preceding block).
	size_t prev_size;
	// Whether this block is allocated or not.
	bool allocated;
} ccm_block_header_t;

// Accessors to visit CCM headers and move between header and data areas.
#define CCM_FIRST_HEADER() ((ccm_block_header_t *) &linker_mstack_vma_end)
#define CCM_NEXT_HEADER(h) (((unsigned int) h) + h->size >= (CCM_BASE + CCM_SIZE) ? (ccm_block_header_t *) 0 : (ccm_block_header_t *) (((char *) h) + h->size))
#define CCM_PREV_HEADER(h) (h->prev_size ? (ccm_block_header_t *) (((char *) h) - h->prev_size) : (ccm_block_header_t *) 0)
#define CCM_HEADER_GET_DATA(h) (((char *) h) + CCM_BLOCK_HEADER_SIZE)
#define CCM_DATA_IS_CCM(d) (((unsigned int) d) >= CCM_BASE && ((unsigned int) d) < CCM_BASE + CCM_SIZE)
#define CCM_DATA_GET_HEADER(d) (CCM_DATA_IS_CCM(d) ? (ccm_block_header_t *) (((char *) d) - CCM_BLOCK_HEADER_SIZE) : (ccm_block_header_t *) 0)

// Whether or not the CCM arena has been initialized.
static volatile bool ccm_arena_initialized = false;



// Nesting level of critical sections.
static unsigned int critical_section_nesting = 0xAAAAAAAAU;



// Current task control block pointer.
extern unsigned long *pxCurrentTCB;



// MPU regions shared by all tasks.
static const struct {
	uint32_t address;
	MPU_RASR_t rasr;
} COMMON_MPU_REGIONS[] = {
	// 0x08000000–0x080FFFFF (length 1 MiB): Flash memory (normal, read-only, write-through cache, executable)
	{ 0x08000000, { .XN = 0, .AP = 0b111, .TEX = 0b000, .S = 0, .C = 1, .B = 0, .SRD = 0, .SIZE = 19, .ENABLE = 1 } },

	// 0x10000000–0x1000FFFF (length 64 kiB): CCM (stack) (normal, read-write, write-back write-allocate cache, not executable)
	{ 0x10000000, { .XN = 1, .AP = 0b011, .TEX = 0b001, .S = 0, .C = 1, .B = 1, .SRD = 0, .SIZE = 15, .ENABLE = 1 } },

	// 0x1FFF0000–0x1FFF7FFF (length 32 kiB): System memory including U_ID and F_SIZE (normal, read-only, write-through cache, not executable)
	{ 0x1FFF0000, { .XN = 1, .AP = 0b111, .TEX = 0b000, .S = 0, .C = 1, .B = 0, .SRD = 0, .SIZE = 14, .ENABLE = 1 } },

	// 0x20000000–0x2001FFFF (length 128 kiB): SRAM (normal, read-write, write-back write-allocate cache, not executable)
	{ 0x20000000, { .XN = 1, .AP = 0b011, .TEX = 0b001, .S = 1, .C = 1, .B = 1, .SRD = 0, .SIZE = 16, .ENABLE = 1 } },

	// 0x40000000–0x4007FFFF (length 512 kiB): Peripherals (device, read-write, not executable) using subregions:
	// Subregion 0 (0x40000000–0x4000FFFF): Enabled (contains APB1)
	// Subregion 1 (0x40010000–0x4001FFFF): Enabled (contains APB2)
	// Subregion 2 (0x40020000–0x4002FFFF): Enabled (contains AHB1)
	// Subregion 3 (0x40030000–0x4003FFFF): Disabled
	// Subregion 4 (0x40040000–0x4004FFFF): Disabled
	// Subregion 5 (0x40050000–0x4005FFFF): Disabled
	// Subregion 6 (0x40060000–0x4006FFFF): Disabled
	// Subregion 7 (0x40070000–0x4007FFFF): Disabled
	{ 0x40000000, { .XN = 1, .AP = 0b011, .TEX = 0b010, .S = 0, .C = 0, .B = 0, .SRD = 0b11111000, .SIZE = 18, .ENABLE = 1 } },

	// 0x50000000–0x5007FFFF (length 512 kiB): Peripherals (device, read-write, not executable) using subregions:
	// Subregion 0 (0x50000000–0x5000FFFF): Enabled (contains AHB2)
	// Subregion 1 (0x50010000–0x5001FFFF): Enabled (contains AHB2)
	// Subregion 2 (0x50020000–0x5002FFFF): Enabled (contains AHB2)
	// Subregion 3 (0x50030000–0x5003FFFF): Enabled (contains AHB2)
	// Subregion 4 (0x50040000–0x5004FFFF): Enabled (contains AHB2)
	// Subregion 5 (0x50050000–0x5005FFFF): Enabled (contains AHB2)
	// Subregion 6 (0x50060000–0x5006FFFF): Enabled (contains AHB2)
	// Subregion 7 (0x50070000–0x5007FFFF): Disabled
	{ 0x50000000, { .XN = 1, .AP = 0b011, .TEX = 0b010, .S = 0, .C = 0, .B = 0, .SRD = 0b10000000, .SIZE = 18, .ENABLE = 1 } },
};

#define STACK_GUARD_MPU_REGION (sizeof(COMMON_MPU_REGIONS) / sizeof(*COMMON_MPU_REGIONS))

static const MPU_RASR_t ZERO_RASR = { .ENABLE = 0 };



void *pvPortMallocAligned(size_t size, void *buffer) {
	// If memory has already been allocated, there is no point doing more allocation.
	if (buffer) {
		return buffer;
	}

	// Initialize CCM arena if not done yet.
	if (!ccm_arena_initialized) {
		ccm_block_header_t *h = CCM_FIRST_HEADER();
		h->size = ((char *) (CCM_BASE + CCM_SIZE)) - (char *) h;
		h->prev_size = 0U;
		h->allocated = false;
		ccm_arena_initialized = true;
	}

	// Round up to nearest multiple of block header size (to keep all block headers aligned), then add block header size to have space for this block’s header.
	size_t needed_size = (size + CCM_BLOCK_HEADER_SIZE - 1U) / CCM_BLOCK_HEADER_SIZE * CCM_BLOCK_HEADER_SIZE + CCM_BLOCK_HEADER_SIZE;

	// Disable interrupts and enable access to guard region while manipulating data structures.
	unsigned long old_imask = portSET_INTERRUPT_MASK_FROM_ISR();
	MPU_RASR_t old_rasr = MPU_RASR;
	MPU_RASR = ZERO_RASR;

	// Find best fit.
	ccm_block_header_t *best = 0;
	for (ccm_block_header_t *i = CCM_FIRST_HEADER(); i; i = CCM_NEXT_HEADER(i)) {
		if (!i->allocated && i->size >= needed_size) {
			if (!best || i->size < best->size) {
				best = i;
			}
		}
	}
	if (best) {
		// Split this block if it’s larger than we need.
		if (best->size > needed_size) {
			ccm_block_header_t *following = CCM_NEXT_HEADER(best);
			if (following) {
				assert(following->allocated); // If this fails, the adjacent-block coalescing algorithm is broken!
			}
			ccm_block_header_t *new = (ccm_block_header_t *) (((char *) best) + needed_size);
			new->size = best->size - needed_size;
			if (following) {
				following->prev_size = new->size;
			}
			new->prev_size = best->size = needed_size;
			new->allocated = false;
		}

		// Mark this block as allocated.
		best->allocated = true;

		buffer = CCM_HEADER_GET_DATA(best);
	}

	// Restore the guard region and interrupts.
	MPU_RASR = old_rasr;
	portCLEAR_INTERRUPT_MASK_FROM_ISR(old_imask);

	// Defer to the main allocator if we are out of CCM.
	if (!buffer) {
		buffer = pvPortMalloc(size);
	}

	return buffer;
}

void vPortFreeAligned(void *buffer) {
	if (CCM_DATA_IS_CCM(buffer)) {
		// Disable interrupts and enable access to guard region while manipulating data structures.
		unsigned long old_imask = portSET_INTERRUPT_MASK_FROM_ISR();
		MPU_RASR_t old_rasr = MPU_RASR;
		MPU_RASR = ZERO_RASR;

		// Convert the data pointer into a header pointer.
		ccm_block_header_t *h = CCM_DATA_GET_HEADER(buffer);

		// Mark this region as free.
		h->allocated = false;

		// Consider coalescing the region.
		ccm_block_header_t *prev = CCM_PREV_HEADER(h);
		ccm_block_header_t *next = CCM_NEXT_HEADER(h);
		if (prev && !prev->allocated) {
			if (next) {
				next->prev_size += prev->size;
			}
			prev->size += h->size;
			h = prev;
		}
		if (next && !next->allocated) {
			ccm_block_header_t *next_next = CCM_NEXT_HEADER(next);
			if (next_next) {
				next_next->prev_size += h->size;
			}
			h->size += next->size;
		}

		// Restore the guard region and interrupts.
		MPU_RASR = old_rasr;
		portCLEAR_INTERRUPT_MASK_FROM_ISR(old_imask);
	} else {
		vPortFree(buffer);
	}
}

static ccm_block_header_t *ccm_find_header(void *data) {
	// Disable interrupts and enable access to guard region while manipulating data structures.
	unsigned long old_imask = portSET_INTERRUPT_MASK_FROM_ISR();
	MPU_RASR_t old_rasr = MPU_RASR;
	MPU_RASR = ZERO_RASR;

	// Scan the headers until finding the right one.
	ccm_block_header_t *best = 0;
	for (ccm_block_header_t *i = CCM_FIRST_HEADER(); i; i = CCM_NEXT_HEADER(i)) {
		if (((char *) i) < (char *) data) {
			best = i;
		} else {
			break;
		}
	}

	// Restore the guard region and interrupts.
	MPU_RASR = old_rasr;
	portCLEAR_INTERRUPT_MASK_FROM_ISR(old_imask);

	return best;
}



unsigned long *pxPortInitialiseStack(unsigned long *tos, TaskFunction_t code, void *params) {
	// xTaskGenericCreate subtracts one word from TOS, then rounds down to alignment.
	// In our case, this means it subtracts two words (eight bytes).
	// ARM CPUs in particular have predecrement stack pointers, so that’s pointless.
	// Fix it, to avoid wasting space!
	tos += 2;

	// Allocate a hardware and software frame on the stack.
	tos -= sizeof(basic_hw_frame_t) / sizeof(*tos);
	basic_hw_frame_t *hwf = (basic_hw_frame_t *) tos;
	tos -= sizeof(basic_sw_frame_t) / sizeof(*tos);
	basic_sw_frame_t *swf = (basic_sw_frame_t *) tos;

	// Fill the basic parts of the frames.
	hwf->r0 = (unsigned long) params;
	hwf->r1 = hwf->r2 = hwf->r3 = hwf->r12 = 0UL;
	hwf->lr = 0xFFFFFFFFUL;
	hwf->return_address = (unsigned long) code;
	hwf->xpsr = 0x01000000UL;
	swf->r4 = swf->r5 = swf->r6 = swf->r7 = swf->r8 = swf->r9 = swf->r10 = swf->r11 = 0UL;
	swf->lr = 0xFFFFFFFDUL; // Return to thread mode, process stack, basic frame
	swf->control = 2; // Run privileged on process stack

	// Compute where the stack overflow guard region should be.
	// This will be the bottom bytes of the stack region.
	if (CCM_DATA_IS_CCM(tos)) {
		// Find out the address of the block header and place the guard region over it.
		ccm_block_header_t *header = ccm_find_header(tos);
		swf->stack_guard_rbar = header;
		{
#define MPU_REGION_SIZE 4U
			_Static_assert(1U << (MPU_REGION_SIZE + 1U) == CCM_BLOCK_HEADER_SIZE, "MPU guard region size does not match CCM block header size!");
			MPU_RASR_t rasr = { .XN = 1, .AP = 0b000, .SRD = 0, .SIZE = MPU_REGION_SIZE, .ENABLE = 1 };
#undef MPU_REGION_SIZE
			swf->stack_guard_rasr = rasr;
		}
	} else {
		// Stack isn’t in CCM, so there will be no guard region.
		swf->stack_guard_rbar = 0;
		{
			MPU_RASR_t rasr = { .ENABLE = 0 };
			swf->stack_guard_rasr = rasr;
		}
	}

	return tos;
}



void __malloc_lock(void) {
	vTaskSuspendAll();
}

void __malloc_unlock(void) {
	xTaskResumeAll();
}



void *pvPortMalloc(size_t size) {
	// Defer to newlib.
	return malloc(size);
}

void vPortFree(void *pv) {
	// Defer to newlib.
	free(pv);
}



long xPortStartScheduler(void) {
	// Set SVCall, PendSV, and systick priorities.
	// SVCall is used to start the scheduler, but is invoked following portDISABLE_INTERRUPTS in vTaskStartScheduler.
	// In ARM, even synchronous interrupts obey the masking registers; in order not to be escalated to hard fault, SVCall must be of an unmasked priority.
	// We never use it for anythign else, so just give it priority above maximum syscall interrupt priority.
	// The others, PendSV and systick, are the normal kernel workhorse interrupts.
	SHPR2.PRI_11 = EXCEPTION_MKPRIO(EXCEPTION_GROUP_PRIO(configMAX_SYSCALL_INTERRUPT_PRIORITY) - 1U, (1U << EXCEPTION_SUB_PRIO_BITS) - 1U);
	SHPR3.PRI_15 = configKERNEL_INTERRUPT_PRIORITY;
	SHPR3.PRI_14 = configKERNEL_INTERRUPT_PRIORITY;

	// We are currently running in process mode on the main stack.
	// What we want to be doing is running a task in process mode on the task stack, with the main stack pointer reset to its initial value ready to handle interrupts.
	// What pxPortInitializeStack builds on the process stack is an exception return frame, as would be built by a PendSV.
	// The easiest way to take advantage of that stack structure to get the first task running is… to perform an exception return!
	// We can’t use PendSV, as that expects to be able to save the current context somewhere first, before loading the new context.
	// It also won’t fix up MSP to clean up the main stack.
	// Use SVCall instead, and let the SVCall handler get everything ready, clean up MSP, and then do an exception return into the first task.
	asm volatile("svc 0");
	__builtin_unreachable();
}



void portENTER_CRITICAL(void) {
	portDISABLE_INTERRUPTS();
	++critical_section_nesting;
}

void portEXIT_CRITICAL(void) {
	if (!--critical_section_nesting) {
		portENABLE_INTERRUPTS();
	}
}



void vPortSVCHandler(void) {
	// These variables must be held in exactly these registers, as they are used below in the inline assembly block.
	register unsigned long tos asm("r0");
	register unsigned long init_stack asm("r1");

	// Initialize critical section nesting count.
	critical_section_nesting = 0U;

	// Enable system timer.
	{
		SYST_RVR_t tmp = { .RELOAD = configCPU_CLOCK_HZ / configTICK_RATE_HZ - 1U };
		SYST_RVR = tmp;
	}
	{
		SYST_CSR_t tmp = { .ENABLE = 1, .TICKINT = 1, .CLKSOURCE = 1 };
		SYST_CSR = tmp;
	}

	// Enable FPU and set up automatic lazy state preservation.
	//
	// Here’s how it will go down:
	//
	// In a task not using the FPU, CONTROL.FPCA will be zero, and on interrupt entry, a basic frame will be stacked.
	// If the ISR uses the FPU, CONTROL.FPCA will be set during that usage, which will also prepare a default context from FPDSCR for the ISR.
	// When the ISR returns, the exception return code will cause CONTROL.FPCA to go back to zero for the task as expected.
	// If the ISR doesn’t use the FPU, CONTROL.FPCA will remain zero throughout, and the exception return will work the same way.
	//
	// In a task using the FPU, on first access, CONTROL.FPCA will be set and a default context will be prepared from FPDSCR.
	// From that moment onward, because CONTROL.FPCA=1 and FPCCR.ASPEN=1, interrupts in that task will always stack extended frames.
	// However, because LSPEN=1, the extended frames will not be immediately populated; rather, on interrupt entry, LSPACT will be set.
	// If the ISR doesn’t use the FPU, LSPACT will remain set throughout.
	// When the ISR returns, the exception return code will indicate an extended frame, but LSPACT=1 will elide restoration of registers.
	// If the ISR does use the FPU, then on first access, the frame will be populated and LSPACT will be cleared, allowing activity to proceed.
	// In that case, on exception return, LSPACT=0 will trigger restoration of registers.
	//
	// Thus, time is spent saving and restoring the FP registers only in the case that both the task *and* the ISR actually use them.
	//
	// This explains ISRs, but what happens during a task switch?
	//
	// When a task not using the FPU takes a PendSV, CONTROL.FPCA will be zero, and on interrupt entry, a basic hardware frame will be stacked.
	// The PendSV ISR will observe that the link register indicates a basic frame, and will not touch the FP registers, stacking only a basic software frame.
	//
	// When a task not using the FPU is being resumed, the PendSV ISR will first load the link register from the basic software frame.
	// Observing that the link register indicates a basic hardware frame, it will conclude that the software frame is also basic and will not touch the FP registers.
	// The exception return will unstack the basic hardware frame and leave CONTROL.FPCA=0.
	//
	// When a task using the FPU takes a PendSV, CONTROL.FPCA will be one, and on interrupt entry, an extended hardware frame will be stacked (though not populated).
	// Because the extended hardware frame is not populated, LSPACT will be set.
	// The PendSV ISR will observe that the link register indicates an extended frame, and will stack the callee-saved FP registers into an extended software frame.
	// The VSTM instruction used to do this is itself an FP instruction, which thus causes the extended hardware frame to be populated and LSPACT to be cleared.
	// Thus, by the time the stack switch occurs, all FP registers will have been placed on the stack (half in the hardware frame and half in the software frame), and LSPACT=0.
	//
	// When a task using the FPU is being resumed, the PendSV ISR will first load the link register from the basic software frame at the beginning of the extended software frame.
	// The PendSV ISR will observe that the link register indicates an extended frame, and will reload the callee-saved FP registers from the extended software frame.
	// The subsequent exception return will unstack the extended hardware frame (restoring the remaining FP registers, since LSPACT=0) and leave CONTROL.FPCA=1.
	{
		FPCCR_t fpccr = { .LSPEN = 1, .ASPEN = 1 };
		FPCCR = fpccr;
		CPACR_t cpacr = CPACR;
		cpacr.CP10 = cpacr.CP11 = 3;
		CPACR = cpacr;
	}
	// Without this, future FP instructions could fail due to not observing the coprocessor enabled yet.
	asm volatile("dsb");
	// An ISB is also necessary, but is omitted here as there is one shortly below.

	// Enable MPU and drop in the common regions.
	for (size_t i = 0; i < sizeof(COMMON_MPU_REGIONS) / sizeof(*COMMON_MPU_REGIONS); ++i) {
		MPU_RNR_t rnr = { .REGION = i };
		MPU_RNR = rnr;
		MPU_RBAR_t rbar = { .REGION = 0, .VALID = 0, .ADDR = COMMON_MPU_REGIONS[i].address >> 5 };
		MPU_RBAR = rbar;
		MPU_RASR = COMMON_MPU_REGIONS[i].rasr;
	}
	{
		MPU_CTRL_t ctrl = {
			.PRIVDEFENA = 0,
			.HFNMIENA = 0,
			.ENABLE = 1,
		};
		MPU_CTRL = ctrl;
	}

	// Leave MPU_RNR set to the CCM stack guard region number.
	// This will be used by the task switcher.
	{
		MPU_RNR_t rnr = { .REGION = STACK_GUARD_MPU_REGION };
		MPU_RNR = rnr;
	}

	// No need to handle an extended software frame here.
	// Because the scheduler is just starting, no tasks have run yet.
	// Every task is always created, initially, with only basic frames.
	// We must also reset the MSP to the start of the main stack (see explanation in xPortStartScheduler).
	tos = *pxCurrentTCB;
	init_stack = *(const unsigned long *) 0x08000000;
	asm volatile(
			// Fix up MSP.
			"msr msp, r1\n\t"
			// Restore software frame and MPU settings.
			"ldmia r0!, {r1-r11, lr}\n\t"
			"msr control, r1\n\t"
			"ldr r1, =0xE000ED9C\n\t" // MPU_RBAR
			"stm r1, {r2-r3}\n\t"
			"dsb\n\t"
			// Fix up PSP and start running the task.
			"msr psp, r0\n\t"
			"isb\n\t"
			"bx lr\n\t"
			:
			// Note that these variables do not need placeholders in the text; they are fixed in r0 and r1 respectively by their asm constraints above.
			: "r" (tos), "r" (init_stack));
	__builtin_unreachable();
}

void vPortPendSVHandler(void) __attribute__((naked));
void vPortPendSVHandler(void) {
	asm volatile(
			// Make software frame on process stack.
			// See the explanation in vPortSVCHandlerImpl for how we deal with floating point registers.
			"ldr r0, =0xE000ED9C\n\t" // MPU_RBAR
			"ldm r0, {r2-r3}\n\t"
			"mrs r0, psp\n\t"
			"mrs r1, control\n\t"
			"tst lr, #16\n\t"
			"it eq\n\t"
			"vstmdbeq r0!, {s16-s31}\n\t"
			"stmdb r0!, {r1-r11, lr}\n\t"
			// Write new top of stack pointer into TCB.
			"ldr r1, =pxCurrentTCB\n\t"
			"ldr r1, [r1]\n\t"
			"str r0, [r1]\n\t"
			// Disable interrupts.
			"mov r0, %[newbasepri]\n\t"
			"msr basepri, r0\n\t"
			// No ISB needed because increasing execution priority serializes instruction execution.
			// Ask scheduler to pick a new task.
			"bl vTaskSwitchContext\n\t"
			// Enable interrupts.
			"mov r0, #0\n\t"
			"msr basepri, r0\n\t"
			// No ISB needed because we don’t care if interrupts are delayed a little longer.
			// Read the new top of stack pointer from TCB.
			"ldr r0, =pxCurrentTCB\n\t"
			"ldr r0, [r0]\n\t"
			"ldr r0, [r0]\n\t"
			// Restore software frame from process stack.
			// See the explanation in vPortSVCHandlerImpl for how we deal with floating point registers.
			"ldmia r0!, {r1-r11, lr}\n\t"
			"tst lr, #16\n\t"
			"it eq\n\t"
			"vldmiaeq r0!, {s16-s31}\n\t"
			"msr control, r1\n\t"
			"ldr r1, =0xE000ED9C\n\t" // MPU_RBAR
			"stm r1, {r2-r3}\n\t"
			"dsb\n\t"
			// Set PSP and return.
			"msr psp, r0\n\t"
			"isb\n\t"
			"bx lr\n\t"
			:
			: [pxCurrentTCB] "i" (&pxCurrentTCB), [newbasepri] "i" (configMAX_SYSCALL_INTERRUPT_PRIORITY));
}

void vPortSysTickHandler(void) {
	portSET_INTERRUPT_MASK_FROM_ISR();
	if(xTaskIncrementTick()) {
		portYIELD_FROM_ISR();
	}
	portCLEAR_INTERRUPT_MASK_FROM_ISR(0U);
}

#if configASSERT_DEFINED == 1
void vPortAssertIfInterruptPriorityInvalid(void) {
	unsigned long current_interrupt;
	asm("mrs %[current_interrupt], ipsr" : [current_interrupt] "=r" (current_interrupt));
	if (current_interrupt >= 16U) {
		// This is a normal hardware interrupt, not a fault or special CPU interrupt.
		// Check the priority.
		current_interrupt -= 16U;
		configASSERT((NVIC_IPR[current_interrupt / 4U] >> (current_interrupt % 4U)) >= configMAX_SYSCALL_INTERRUPT_PRIORITY);
	}
}
#endif

