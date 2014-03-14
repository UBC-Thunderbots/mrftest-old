#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <registers/scb.h>

// The type of words in a stack.
typedef unsigned long StackType_t;

// The general-purpose type for an ordinary integer.
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

// The type of a tick counter.
typedef unsigned long TickType_t;

// The infinity value for a tick count.
#define portMAX_DELAY ULONG_MAX

// The stack grows down.
#define portSTACK_GROWTH -1

// The tick interval, in milliseconds.
#define portTICK_PERIOD_MS (1000UL / configTICK_RATE_HZ)

// Stacks will be eight-byte aligned.
#define portBYTE_ALIGNMENT 8U

// The type of a task function.
#define portTASK_FUNCTION_PROTO(fn, params) void fn(void *params)
#define portTASK_FUNCTION portTASK_FUNCTION_PROTO

// The possible system call codes.
typedef enum {
	PORT_SYSCALL_YIELD,
	PORT_SYSCALL_DISABLE_INTERRUPTS,
	PORT_SYSCALL_ENABLE_INTERRUPTS,
	PORT_SYSCALL_RAISE_PRIVILEGE,
	PORT_SYSCALL_ENABLE_HW_INTERRUPT,
	PORT_SYSCALL_DISABLE_HW_INTERRUPT,
	PORT_SYSCALL_START_SCHEDULER,
} portSyscallType;

// Functions related to yielding tasks.
static void portYIELD(void) {
	// Yielding from userspace is done with an ordinary system call.
	asm volatile("svc %[code]" :: [code] "i" (PORT_SYSCALL_YIELD));
}
#define portYIELD_WITHIN_API portYIELD
static void portYIELD_FROM_ISR(void) {
	// Yielding from an ISR is done by pending the PendSV interrupt and returning, causing a priority de-escalation followed by PendSV handler invocation.
	ICSR_t tmp = { .PENDSVSET = 1 };
	ICSR = tmp;
	// DSB ensures the write completes in a timely manner and won’t return to userspace and execute more instructions before taking the PendSV.
	asm volatile("dsb");
	// ISB is not necessary as exception return implicitly contains an ISB.
}

// Functions related to allocating and freeing task stacks.
void *pvPortMallocAligned(size_t size, void *buffer);
void vPortFreeAligned(void *buffer);
#define pvPortMallocAligned pvPortMallocAligned
#define vPortFreeAligned vPortFreeAligned

// These functions enable and disable interrupts from non-ISR code; they need privilege and are therefore syscalls.
static void portENABLE_INTERRUPTS(void) { __atomic_signal_fence(__ATOMIC_RELEASE); asm volatile("svc %[code]" :: [code] "i" (PORT_SYSCALL_ENABLE_INTERRUPTS)); }
static void portDISABLE_INTERRUPTS(void) { asm volatile("svc %[code]" :: [code] "i" (PORT_SYSCALL_DISABLE_INTERRUPTS)); __atomic_signal_fence(__ATOMIC_ACQUIRE); }

// These functions are basically nestable variants of the functions above, which just maintain a counter and call those functions.
void portENTER_CRITICAL(void);
void portEXIT_CRITICAL(void);

// These functions enable and disable interrupts from an ISR; ISRs are always privileged and therefore these are implemented directly.
// Also, we do not need to save a prior value of BASEPRI.
// Exception entry does not modify BASEPRI, so it will normally be zero.
// Actual execution priority is determined from the lesser of the values implied by BASEPRI, PRIMASK, FAULTMASK, and the active exception set.
// So, ISR entry just modifies the active exception set and that modifies the execution priority of the CPU and prevents inappropriate preemption.
// What this means for us is that we can disable unwanted interrupts by setting BASEPRI to an appropriate value, then re-enable them by setting it to zero.
// We won’t reenter ourself, because the active exception set takes care of that (and is, in fact, the usual mechanism for doing so).
// Therefore no old mask value needs to be saved at all!
static unsigned long portSET_INTERRUPT_MASK_FROM_ISR(void) {
	asm volatile("msr basepri, %[newpri]" :: [newpri] "r" (configMAX_SYSCALL_INTERRUPT_PRIORITY));
	// ARMv7-M manual section B5.2.3 states:
	// “If execution of a MSR instruction increases the execution priority, the MSR execution serializes that change to the instruction stream.”
	// Consequently, no ISB is needed, as we are increasing execution priority.
	// However, we must force the compiler not to reorder subsequent memory operations backwards past the disable, so a fence is needed.
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
	return 0UL;
}
static void portCLEAR_INTERRUPT_MASK_FROM_ISR(unsigned long old __attribute__((unused))) {
	// Force the compiler not to reorder prior memory operations past the enable.
	__atomic_signal_fence(__ATOMIC_RELEASE);
	asm volatile("msr basepri, %[zero]" :: [zero] "r" (0UL));
	// ARMv7-M manual section B5.2.3 states:
	// “If execution of a MSR instruction decreases the execution priority, the architecture guarantees only that the new priority is visible to instructions executed after either executing an ISB, or performing an exception entry or exception return.”
	// So, we must issue an ISB.
	// Normally we could just say who cares, let the interrupts be taken a few instructions later.
	// However, conceivably, there could be a situation where interrupts are enabled then immediately re-disabled, in an attempt to flush out pending interrupts.
	// Such an operation might fail without the ISB, as it might actually *never* observe interrupts enabled!
	// So, just issue the ISB to be safe.
	asm volatile("isb");
}
#define portSET_INTERRUPT_MASK_FROM_ISR portSET_INTERRUPT_MASK_FROM_ISR
#define portCLEAR_INTERRUPT_MASK_FROM_ISR portCLEAR_INTERRUPT_MASK_FROM_ISR

// These functions turn a specific hardware interrupt on and off.
static void portENABLE_HW_INTERRUPT(unsigned int irq, unsigned int priority) {
	register unsigned int i asm("r0") = irq;
	register unsigned int j asm("r1") = priority;
	asm volatile("svc %[service]" :: [service] "i" (PORT_SYSCALL_ENABLE_HW_INTERRUPT), "r" (i), "r" (j));
}
static void portDISABLE_HW_INTERRUPT(unsigned int irq) {
	register unsigned int i asm("r0") = irq;
	asm volatile("svc %[service]" :: [service] "i" (PORT_SYSCALL_DISABLE_HW_INTERRUPT), "r" (i));
}

// These functions take and drop privileges.
static void portRAISE_PRIVILEGE(void) {
	asm volatile("svc %[service]" :: [service] "i" (PORT_SYSCALL_RAISE_PRIVILEGE));
}
static void portDROP_PRIVILEGE(void) {
	unsigned long control;
	asm("mrs %[control], control" : [control] "=r" (control));
	asm volatile("msr control, %[control]" :: [control] "r" (control | 1U));
}
static bool portHAS_PRIVILEGE(void) {
	unsigned long control;
	asm("mrs %[control], control" : [control] "=r" (control));
	return !(control & 1U);
}

// Interrupt handlers which the application must wire into the vector table.
void vPortSVCHandler(void);
void vPortPendSVHandler(void);
void vPortSysTickHandler(void);

// Functionality used by tasks.c to optimize task selection if requested.
#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
#if configMAX_PRIORITIES > 32
#error Cannot use optimized task selection with more than 32 priorities!
#endif
#define portRECORD_READY_PRIORITY(priority, ready_priorities) ((ready_priorities) |= (1UL << (priority)))
#define portRESET_READY_PRIORITY(priority, ready_priorities) ((ready_priorities) &= ~(1UL << (priority)))
#define portGET_HIGHEST_PRIORITY(top_priority, ready_priorities) ((top_priority) = (31U - __builtin_clz((ready_priorities))))
#endif

#if configASSERT_DEFINED == 1
// Checks if the CPU is currently running at a legal interrupt level to call FreeRTOS functions.
void vPortAssertIfInterruptPriorityInvalid(void);
#define portASSERT_IF_INTERRUPT_PRIORITY_INVALID vPortAssertIfInterruptPriorityInvalid
#endif

#endif

