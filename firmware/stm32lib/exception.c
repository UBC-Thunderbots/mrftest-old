#include <exception.h>
#include <registers/mpu.h>
#include <registers/nvic.h>
#include <registers/scb.h>
#include <sleep.h>
#include <string.h>

static const exception_core_writer_t *core_writer = 0;
static const exception_app_cbs_t *app_cbs = 0;

void exception_init(const exception_core_writer_t *cw, const exception_app_cbs_t *acbs) {
	// We will run as follows:
	// CPU exceptions (UsageFault, BusFault, MemManage) will be priority 0, subpriority 0 and thus preempt everything else.
	// Hardware interrupts will be priority 1, subpriority 0.
	// PendSV exceptions will be priority 1, subpriority 0x3F so they neither preempt nor are preempted by hardware interrupts but are taken at less priority.
	// Set hardware interrupt priorities.
	for (size_t i = 0; i < sizeof(NVIC_IPR) / sizeof(*NVIC_IPR); ++i) {
		NVIC_IPR[i] = 0x40404040;
	}

	// Set PendSV (exception 14) priority.
	SHPR3.PRI_14 = 0x7F;

	// Enable Usage, Bus, and MemManage faults to be taken as such rather than escalating to HardFaults.
	{
		SHCSR_t tmp = SHCSR;
		tmp.USGFAULTENA = 1;
		tmp.BUSFAULTENA = 1;
		tmp.MEMFAULTENA = 1;
		SHCSR = tmp;
	}

	// Enable trap on divide by zero.
	CCR.DIV_0_TRP = 1;

	// Register the core dump writer and application callbacks.
	core_writer = cw;
	app_cbs = acbs;
}

typedef struct __attribute__((packed)) {
	char ei_mag[4];
	uint8_t ei_class;
	uint8_t ei_data;
	uint8_t ei_version;
	uint8_t ei_osabi;
	uint8_t ei_abiversion;
	uint8_t ei_pad[7];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;
	uint32_t e_phoff;
	uint32_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
} elf_file_header_t;

typedef struct __attribute__((packed)) {
	uint32_t p_type;
	uint32_t p_offset;
	uint32_t p_vaddr;
	uint32_t p_paddr;
	uint32_t p_filesz;
	uint32_t p_memsz;
	uint32_t p_flags;
	uint32_t p_align;
} elf_program_header_t;

typedef struct __attribute__((packed)) {
	uint32_t n_namesz;
	uint32_t n_descsz;
	uint32_t n_type;
} elf_note_header_t;

typedef struct __attribute__((packed)) {
	uint32_t si_signo;
	uint32_t si_code;
	uint32_t si_errno;
	uint32_t pr_cursig;
	uint32_t pr_sigpend;
	uint32_t pr_sighold;
	uint32_t pr_pid;
	uint32_t pr_ppid;
	uint32_t pr_pgrp;
	uint32_t pr_sid;
	uint64_t pr_utime;
	uint64_t pr_stime;
	uint64_t pr_cutime;
	uint64_t pr_cstime;
	uint32_t pr_gpregs[13];
	uint32_t pr_reg_sp;
	uint32_t pr_reg_lr;
	uint32_t pr_reg_pc;
	uint32_t pr_reg_xpsr;
	uint32_t pr_reg_orig_r0;
	uint32_t pr_fpvalid;
} elf_note_prstatus_t;

typedef struct __attribute__((packed)) {
	uint32_t si_signum;
	uint32_t si_errno;
	uint32_t si_code;
	uint32_t si_addr;
	uint32_t padding[28];
} elf_note_pr_siginfo_t;

typedef struct __attribute__((packed)) {
	uint32_t sregs[32];
	uint32_t padding[32];
	uint32_t fpscr;
} elf_note_arm_vfp_t;

typedef struct __attribute__((packed)) {
	elf_file_header_t file_header;
	elf_program_header_t note_pheader;
	elf_program_header_t ccm_pheader;
	elf_program_header_t ram_pheader;
} elf_headers_t;

typedef struct __attribute__((packed)) {
	elf_note_header_t prstatus_nheader;
	char prstatus_name[8];
	elf_note_prstatus_t prstatus;

	elf_note_header_t pr_siginfo_nheader;
	char pr_siginfo_name[8];
	elf_note_pr_siginfo_t pr_siginfo;

	elf_note_header_t arm_vfp_nheader;
	char arm_vfp_name[8];
	elf_note_arm_vfp_t arm_vfp;
} elf_notes_t;

static const elf_headers_t ELF_HEADERS = {
	.file_header = {
		.ei_mag = { 0x7F, 'E', 'L', 'F' },
		.ei_class = 1, // ELFCLASS32
		.ei_data = 1, // ELFDATA2LSB
		.ei_version = 1, // EV_CURRENT
		.ei_osabi = 0, // ELFOSABI_NONE
		.ei_abiversion = 0,
		.ei_pad = { 0 },

		.e_type = 4, // ET_CORE
		.e_machine = 40, // EM_ARM
		.e_version = 1, // EV_CURRENT
		.e_entry = 0,
		.e_phoff = sizeof(elf_file_header_t),
		.e_shoff = 0,
		.e_flags = 0x05400400, // EF_ARM_EABI_VER5 | EF_ARM_LE8 | EF_ARM_VFP_FLOAT
		.e_ehsize = sizeof(elf_file_header_t),
		.e_phentsize = sizeof(elf_program_header_t),
		.e_phnum = 3,
		.e_shentsize = 0,
		.e_shnum = 0,
		.e_shstrndx = 0,
	},

	.note_pheader = {
		.p_type = 4, // PT_NOTE
		.p_offset = sizeof(elf_headers_t),
		.p_vaddr = 0,
		.p_paddr = 0,
		.p_filesz = sizeof(elf_notes_t),
		.p_memsz = 0,
		.p_flags = 0,
		.p_align = 0,
	},

	.ccm_pheader = {
		.p_type = 1, // PT_LOAD
		.p_offset = sizeof(elf_headers_t) + sizeof(elf_notes_t),
		.p_vaddr = 0x10000000,
		.p_paddr = 0x10000000,
		.p_filesz = 64 * 1024,
		.p_memsz = 64 * 1024,
		.p_flags = 6, // PF_R | PF_W
		.p_align = 4,
	},

	.ram_pheader = {
		.p_type = 1, // PT_LOAD
		.p_offset = sizeof(elf_headers_t) + sizeof(elf_notes_t) + 64 * 1024,
		.p_vaddr = 0x20000000,
		.p_paddr = 0x20000000,
		.p_filesz = 128 * 1024,
		.p_memsz = 128 * 1024,
		.p_flags = 6, // PF_R | PF_W
		.p_align = 4,
	},
};

static elf_notes_t elf_notes = {
	.prstatus_nheader = {
		.n_namesz = 5,
		.n_descsz = sizeof(elf_note_prstatus_t),
		.n_type = 1, // NT_PRSTATUS
	},
	.prstatus_name = "CORE",

	.pr_siginfo_nheader = {
		.n_namesz = 5,
		.n_descsz = sizeof(elf_note_pr_siginfo_t),
		.n_type = 0x53494749, // IGIS
	},
	.pr_siginfo_name = "CORE",

	.arm_vfp_nheader = {
		.n_namesz = 6,
		.n_descsz = sizeof(elf_note_arm_vfp_t),
		.n_type = 0x400, // NT_ARM_VFP
	},
	.arm_vfp_name = "LINUX",
};

typedef struct __attribute__((packed)) {
	uint32_t xpsr;
	uint32_t msp;
	uint32_t psp;
	uint32_t primask;
	uint32_t basepri;
	uint32_t faultmask;
	uint32_t control;
	uint32_t gpregs[13];
	uint32_t lr;
} sw_stack_frame_t;

typedef struct __attribute__((packed)) {
	uint32_t r0;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r12;
	uint32_t lr;
	uint32_t pc;
	uint32_t xpsr;
} hw_basic_stack_frame_t;

static void fill_core_notes(const sw_stack_frame_t *swframe, unsigned int cause) {
	// Fill out the signal info based on the exception number and fault status registers.
	switch (cause) {
		case 3: // Hard fault
			if (HFSR.DEBUGEVT) {
				elf_notes.pr_siginfo.si_signum = 6; // SIGABRT
				elf_notes.pr_siginfo.si_code = -6; // Sent by tkill (what abort()/assert() do)
			} else {
				elf_notes.pr_siginfo.si_signum = 9; // SIGKILL (not really any good answer here)
				elf_notes.pr_siginfo.si_code = 0x80; // SI_KERNEL
			}
			break;

		case 4: // Memory manage fault
			elf_notes.pr_siginfo.si_signum = 11; // SIGSEGV
			if (CFSR.MMARVALID) {
				elf_notes.pr_siginfo.si_addr = (uint32_t) MMFAR;
			}
			// We don’t bother figuring out whether we had SEGV_MAPERR or SEGV_ACCERR.
			// So, leave si_code at zero.
			break;

		case 5: // Bus fault
			elf_notes.pr_siginfo.si_signum = 7; // SIGBUS
			if (CFSR.BFARVALID) {
				elf_notes.pr_siginfo.si_addr = (uint32_t) BFAR;
			}
			elf_notes.pr_siginfo.si_code = 3; // BUS_OBJERR
			break;

		case 6: // Usage fault
			if (CFSR.DIVBYZERO) {
				elf_notes.pr_siginfo.si_signum = 8; // SIGFPE
				elf_notes.pr_siginfo.si_code = 1; // FPE_INTDIV
			} else if (CFSR.UNALIGNED) {
				elf_notes.pr_siginfo.si_signum = 7; // SIGBUS
				elf_notes.pr_siginfo.si_code = 1; // BUS_ADRALN
			} else if (CFSR.NOCP) {
				elf_notes.pr_siginfo.si_signum = 4; // SIGILL
				elf_notes.pr_siginfo.si_code = 7; // ILL_COPROC
			} else if (CFSR.INVPC) {
				elf_notes.pr_siginfo.si_signum = 4; // SIGILL
				elf_notes.pr_siginfo.si_code = 2; // ILL_ILLOPN
			} else if (CFSR.INVSTATE) {
				elf_notes.pr_siginfo.si_signum = 4; // SIGILL
				elf_notes.pr_siginfo.si_code = 2; // ILL_ILLOPN
			} else if (CFSR.UNDEFINSTR) {
				elf_notes.pr_siginfo.si_signum = 4; // SIGILL
				elf_notes.pr_siginfo.si_code = 1; // ILL_ILLOPC
			}
			break;
	}

	// Copy signal info over to prstatus.
	elf_notes.prstatus.si_signo = elf_notes.pr_siginfo.si_signum;
	elf_notes.prstatus.si_code = elf_notes.pr_siginfo.si_code;
	elf_notes.prstatus.si_errno = elf_notes.pr_siginfo.si_code;
	elf_notes.prstatus.pr_cursig = elf_notes.pr_siginfo.si_signum;

	// General purpose registers always come from the software frame.
	// The hardware frame may or may not exist, and if it does, it only has a few GP regs.
	// The software frame *always* exists and has *all* the GP regs, so it’s much easier.
	memcpy(&elf_notes.prstatus.pr_gpregs, &swframe->gpregs, 13 * sizeof(uint32_t));

	{
		// Start by computing the hardware SP address based on the exception return code.
		// The exception return code itself is saved in the software frame’s link register.
		const hw_basic_stack_frame_t *hwframe;
		if (swframe->lr & 4) {
			// This means we were running on the process stack.
			elf_notes.prstatus.pr_reg_sp = swframe->psp + sizeof(hw_basic_stack_frame_t);

			// However, there might have been an error while stacking the exception frame.
			// This could be caused by e.g. a stack overflow, leaving no space for the frame.
			// In that case, we don’t want to go poking around there!
			// We might take another fault, and anyway the values we get would be useless.
			// So check if we faulted while stacking the exception frame before using it.
			if (CFSR.MSTKERR || CFSR.STKERR) {
				// A stacking error occurred.
				hwframe = 0;
			} else {
				// No stacking error occurred.
				// The hardware frame was the last thing pushed onto the process stack.
				// It must therefore be pointed to by PSP.
				hwframe = (const hw_basic_stack_frame_t *) swframe->psp;
			}

			if (hwframe && hwframe->xpsr & (1U << 9)) {
				// Stack pointer was adjusted by 4 to achieve 8-byte alignment.
				elf_notes.prstatus.pr_reg_sp += 4;
			}

			// We call userspace PID 1.
			elf_notes.prstatus.pr_pid = 1;
		} else {
			// This means we were running on the process stack.
			// We assume the main stack is always OK.
			// It must be, because we’re using it right now!
			// Point the hardware frame pointer at the frame on the main stack.
			// This will have been pushed right before the transfer to the exception ISR.
			// The exception ISR will then immediately have pushed the software frame.
			// Thus, the hardware frame lives immediately above the software frame.
			hwframe = (const hw_basic_stack_frame_t *) (swframe + 1);
			elf_notes.prstatus.pr_reg_sp = (uint32_t) (hwframe + 1);
			if (hwframe->xpsr & (1U << 9)) {
				// Stack pointer was adjusted by 4 to achieve 8-byte alignment.
				elf_notes.prstatus.pr_reg_sp += 4;
			}

			// We call kernelspace PID 0.
			elf_notes.prstatus.pr_pid = 0;
		}

		// Fill the rest of prstatus as best we can.
		if (hwframe) {
			elf_notes.prstatus.pr_reg_lr = hwframe->lr;
			elf_notes.prstatus.pr_reg_pc = hwframe->pc;
			elf_notes.prstatus.pr_reg_xpsr = hwframe->xpsr;
		} else {
			elf_notes.prstatus.pr_reg_xpsr = swframe->xpsr;
		}
	}

	// Do some final tidying up.
	elf_notes.prstatus.pr_reg_orig_r0 = elf_notes.prstatus.pr_gpregs[0];
}

static void common_fault_handler(const sw_stack_frame_t *sp, unsigned int cause) __attribute__((noreturn, used));
static void common_fault_handler(const sw_stack_frame_t *sp, unsigned int cause) {
	// Call the early application callbacks.
	if (app_cbs && app_cbs->early) {
		app_cbs->early();
	}

	// Turn off the memory protection unit.
	MPU_CTRL.ENABLE = 0;
	asm volatile("dsb");
	asm volatile("isb");

	// If we have a core dump writer, then write a core dump.
	bool core_written = false;
	if (core_writer) {
		fill_core_notes(sp, cause);
		core_writer->start();
		core_writer->write(&ELF_HEADERS, sizeof(ELF_HEADERS));
		core_writer->write(&elf_notes, sizeof(elf_notes));
		core_writer->write((const void *) 0x10000000, 64 * 1024);
		core_writer->write((const void *) 0x20000000, 128 * 1024);
		core_written = core_writer->end();
	}

	// Call the late application callback.
	if (app_cbs && app_cbs->late) {
		app_cbs->late(core_written);
	}

	// Late application callback return or not provided.
	// Die forever.
	for (;;) {
		asm volatile("wfi");
	}
}

#define GEN_FAULT_HANDLER(name, cause) \
	void exception_ ## name ## _fault_vector(void) { \
		asm volatile( \
				"push {r0-r12, lr}\n\t" \
				"mrs r0, xpsr\n\t" \
				"mrs r1, msp\n\t" \
				"mrs r2, psp\n\t" \
				"mrs r3, primask\n\t" \
				"mrs r4, basepri\n\t" \
				"mrs r5, faultmask\n\t" \
				"mrs r6, control\n\t" \
				"push {r0-r6}\n\t" \
				"mov r0, sp\n\t" \
				"mov r1, %[cause_constant]\n\t" \
				"b common_fault_handler\n\t" \
				: \
				: [cause_constant] "i" (cause)); \
		__builtin_unreachable(); \
	}

GEN_FAULT_HANDLER(hard, 3)
GEN_FAULT_HANDLER(memory_manage, 4)
GEN_FAULT_HANDLER(bus, 5)
GEN_FAULT_HANDLER(usage, 6)

