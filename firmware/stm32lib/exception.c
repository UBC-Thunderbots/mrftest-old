#include <exception.h>
#include <registers.h>
#include <stdlib.h>

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
	SCS_SHPR3 = (SCS_SHPR3 & ~0x00FF0000) | 0x007F0000;

	// Enable Usage, Bus, and MemManage faults to be taken as such rather than escalating to HardFaults.
	SCS_SHCSR |= USGFAULTENA | BUSFAULTENA | MEMFAULTENA;
}

static void hard_fault_vector_impl(const uint32_t *context) __attribute__((used));
static void hard_fault_vector_impl(const uint32_t *context) {
	abort_cause.cause = ABORT_CAUSE_HARD_FAULT;
	abort_cause.detail[0] = SCS_HFSR; // Fault status register
	abort_cause.detail[1] = SCS_CFSR; // This may contain the original underlying cause of the hard fault
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
	abort_cause.detail[0] = SCS_CFSR; // Fault status register
	abort_cause.detail[1] = SCS_MMFAR; // Faulting data address
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
	abort_cause.detail[0] = SCS_CFSR; // Fault status register
	abort_cause.detail[1] = SCS_BFAR; // Faulting data address
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
	abort_cause.detail[0] = SCS_CFSR; // Fault status register
	abort_cause.detail[1] = SCS_BFAR; // Faulting data address
	abort_cause.detail[2] = context[6]; // Return address to faulting instruction
	abort();
}

void exception_usage_fault_vector(void) {
	asm volatile("mov r0, sp");
	asm volatile("b usage_fault_vector_impl");
}

