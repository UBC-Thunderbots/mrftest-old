#include <exception.h>
#include <abort.h>
#include <registers/nvic.h>
#include <registers/scb.h>
#include <string.h>

void exception_init(void) {
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
}

static void hard_fault_vector_impl(const uint32_t *context) __attribute__((used));
static void hard_fault_vector_impl(const uint32_t *context) {
	abort_cause.cause = ABORT_CAUSE_HARD_FAULT;
	{
		HFSR_t tmp = HFSR;
		memcpy(&abort_cause.detail[0], &tmp, sizeof(uint32_t)); // Fault status register
	}
	{
		CFSR_t tmp = CFSR;
		memcpy(&abort_cause.detail[1], &tmp, sizeof(uint32_t)); // This may contain the original underlying cause of the hard fault
	}
	abort_cause.detail[2] = context[6]; // Return address to faulting instruction
	abort();
}

void exception_hard_fault_vector(void) {
	asm volatile("mov r0, sp");
	asm volatile("b hard_fault_vector_impl");
}

static void memory_manage_fault_vector_impl(const uint32_t *context) __attribute__((used));
static void memory_manage_fault_vector_impl(const uint32_t *context) {
	abort_cause.cause = ABORT_CAUSE_MEMORY_MANAGE_FAULT;
	{
		CFSR_t tmp = CFSR;
		memcpy(&abort_cause.detail[0], &tmp, sizeof(uint32_t)); // Fault status register
	}
	abort_cause.detail[1] = (uint32_t) MMFAR; // Faulting data address
	abort_cause.detail[2] = context[6]; // Return address to faulting instruction
	abort();
}

void exception_memory_manage_fault_vector(void) {
	asm volatile("mov r0, sp");
	asm volatile("b memory_manage_fault_vector_impl");
}

static void bus_fault_vector_impl(const uint32_t *context) __attribute__((used));
static void bus_fault_vector_impl(const uint32_t *context) {
	abort_cause.cause = ABORT_CAUSE_BUS_FAULT;
	{
		CFSR_t tmp = CFSR;
		memcpy(&abort_cause.detail[0], &tmp, sizeof(uint32_t)); // Fault status register
	}
	abort_cause.detail[1] = (uint32_t) BFAR; // Faulting data address
	abort_cause.detail[2] = context[6]; // Return address to faulting instruction
	abort();
}

void exception_bus_fault_vector(void) {
	asm volatile("mov r0, sp");
	asm volatile("b bus_fault_vector_impl");
}

static void usage_fault_vector_impl(const uint32_t *context) __attribute__((used));
static void usage_fault_vector_impl(const uint32_t *context) {
	abort_cause.cause = ABORT_CAUSE_USAGE_FAULT;
	{
		CFSR_t tmp = CFSR;
		memcpy(&abort_cause.detail[0], &tmp, sizeof(uint32_t)); // Fault status register
	}
	abort_cause.detail[1] = (uint32_t) BFAR; // Faulting data address
	abort_cause.detail[2] = context[6]; // Return address to faulting instruction
	abort();
}

void exception_usage_fault_vector(void) {
	asm volatile("mov r0, sp");
	asm volatile("b usage_fault_vector_impl");
}

