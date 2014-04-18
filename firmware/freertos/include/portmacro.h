#ifndef PORTMACRO_H
#define PORTMACRO_H

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <registers/nvic.h>
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

// Functions related to yielding tasks.
static void portYIELD(void) {
	// Whether in an ISR or not, yielding is done by pending the PendSV interrupt.
	// In an ISR, this will be at lower priority, so on exception return, priority will de-escalate.
	// In a task, this will be taken immediately.
	ICSR_t tmp = { .PENDSVSET = 1 };
	ICSR = tmp;
	// DSB ensures the write completes in a timely manner and won’t return to userspace and execute more instructions before taking the PendSV.
	asm volatile("dsb");
	// ISB ensures no further instructions execute until after the effects of the PendSV occur.
	asm volatile("isb");
}
#define portYIELD_WITHIN_API portYIELD
#define portYIELD_FROM_ISR portYIELD

// Functions related to allocating and freeing task stacks.
void *pvPortMallocAligned(size_t size, void *buffer);
void vPortFreeAligned(void *buffer);
#define pvPortMallocAligned pvPortMallocAligned
#define vPortFreeAligned vPortFreeAligned

// These functions enable and disable interrupts from ISR or non-ISR code.
static inline void portENABLE_INTERRUPTS(void) {
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
static inline void portDISABLE_INTERRUPTS(void) {
	asm volatile("msr basepri, %[newpri]" :: [newpri] "r" (configMAX_SYSCALL_INTERRUPT_PRIORITY));
	// ARMv7-M manual section B5.2.3 states:
	// “If execution of a MSR instruction increases the execution priority, the MSR execution serializes that change to the instruction stream.”
	// Consequently, no ISB is needed, as we are increasing execution priority.
	// However, we must force the compiler not to reorder subsequent memory operations backwards past the disable, so a fence is needed.
	__atomic_signal_fence(__ATOMIC_ACQUIRE);
}

// These functions are basically nestable variants of the functions above, which just maintain a counter and call those functions.
void portENTER_CRITICAL(void);
void portEXIT_CRITICAL(void);

// These functions enable and disable interrupts from an ISR.
// We do not need to save a prior value of BASEPRI.
// Exception entry does not modify BASEPRI, so it will normally be zero.
// Actual execution priority is determined from the lesser of the values implied by BASEPRI, PRIMASK, FAULTMASK, and the active exception set.
// So, ISR entry just modifies the active exception set and that modifies the execution priority of the CPU and prevents inappropriate preemption.
// What this means for us is that we can disable unwanted interrupts by setting BASEPRI to an appropriate value, then re-enable them by setting it to zero.
// We won’t reenter ourself, because the active exception set takes care of that (and is, in fact, the usual mechanism for doing so).
// Therefore no old mask value needs to be saved at all!
static unsigned long portSET_INTERRUPT_MASK_FROM_ISR(void) {
	portDISABLE_INTERRUPTS();
	return 0UL;
}
static void portCLEAR_INTERRUPT_MASK_FROM_ISR(unsigned long old __attribute__((unused))) {
	portENABLE_INTERRUPTS();
}
#define portSET_INTERRUPT_MASK_FROM_ISR portSET_INTERRUPT_MASK_FROM_ISR
#define portCLEAR_INTERRUPT_MASK_FROM_ISR portCLEAR_INTERRUPT_MASK_FROM_ISR

// These functions turn a specific hardware interrupt on and off.
static void portENABLE_HW_INTERRUPT(unsigned int irq, unsigned int priority) {
	unsigned int ipr_index = irq / 4U;
	unsigned int ipr_shift_dist = (irq % 4U) * 8U;
	NVIC_IPR[ipr_index] = (NVIC_IPR[ipr_index] & ~(0xFFU << ipr_shift_dist)) | (priority << ipr_shift_dist);
	// We must barrier to ensure the priority is set before the interrupt is enabled.
	// However, we do not need a synchronization barrier because we don’t care *when*, specifically, in instruction stream order, the effects become visible, as long as they do so in the right order.
	asm volatile("dmb");
	NVIC_ISER[irq / 32U] = 1U << (irq % 32U);
}
static void portDISABLE_HW_INTERRUPT(unsigned int irq) {
	NVIC_ICER[irq / 32U] = 1U << (irq % 32U);
	// The caller may expect that the interrupt cannot possibly happen once the call is complete.
	// DSB ensures the write is completed before proceeding.
	asm volatile("dsb");
	// ISB ensures no subsequent instructions are acted on with the interrupt still enabled.
	asm volatile("isb");
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

