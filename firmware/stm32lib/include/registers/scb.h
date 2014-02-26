#ifndef STM32LIB_REGISTERS_SCB_H
#define STM32LIB_REGISTERS_SCB_H

/**
 * \file
 *
 * \brief Defines the system control block registers.
 */

typedef struct {
	unsigned DISMCYCNT : 1;
	unsigned DISDEFWBUF : 1;
	unsigned DISFOLD : 1;
	unsigned : 5;
	unsigned DISFPCA : 1;
	unsigned DISOOFP : 1;
	unsigned : 22;
} ACTLR_t;
_Static_assert(sizeof(ACTLR_t) == 4U, "ACTLR_t is wrong size");
#define ACTLR (*(volatile ACTLR_t *) 0xE000E008)

typedef struct {
	unsigned REVISION : 4;
	unsigned PARTNO : 12;
	unsigned CONSTANT : 4;
	unsigned VARIANT : 4;
	unsigned IMPLEMENTER : 8;
} CPUID_t;
_Static_assert(sizeof(CPUID_t) == 4U, "CPUID_t is wrong size");
#define CPUID (*(volatile CPUID_t *) 0xE000ED00)

typedef struct {
	unsigned VECTACTIVE : 9;
	unsigned : 2;
	unsigned RETTOBASE : 1;
	unsigned VECTPENDING : 10;
	unsigned ISRPENDING : 1;
	unsigned : 2;
	unsigned PENDSTCLR : 1;
	unsigned PENDSTSET : 1;
	unsigned PENDSVCLR : 1;
	unsigned PENDSVSET : 1;
	unsigned : 2;
	unsigned NMIPENDSET : 1;
} ICSR_t;
_Static_assert(sizeof(ICSR_t) == 4U, "ICSR_t is wrong size");
#define ICSR (*(volatile ICSR_t *) 0xE000ED04)

#define VTOR (*(void * volatile *) 0xE000ED08)

typedef struct {
	unsigned VECTRESET : 1;
	unsigned VECTCLRACTIVE : 1;
	unsigned SYSRESETREQ : 1;
	unsigned : 5;
	unsigned PRIGROUP : 3;
	unsigned : 4;
	unsigned ENDIANNESS : 1;
	unsigned VECTKEY : 16;
} AIRCR_t;
_Static_assert(sizeof(AIRCR_t) == 4U, "AIRCR_t is wrong size");
#define AIRCR (*(volatile AIRCR_t *) 0xE000ED0C)

typedef struct {
	unsigned : 1;
	unsigned SLEEPONEXIT : 1;
	unsigned SLEEPDEEP : 1;
	unsigned : 1;
	unsigned SEVONPEND : 1;
	unsigned : 27;
} SCR_t;
_Static_assert(sizeof(SCR_t) == 4U, "SCR_t is wrong size");
#define SCR (*(volatile SCR_t *) 0xE000ED10)

typedef struct {
	unsigned NONBASETHRDENA : 1;
	unsigned USERSETMPEND : 1;
	unsigned : 1;
	unsigned UNALIGN_TRP : 1;
	unsigned DIV_0_TRP : 1;
	unsigned : 3;
	unsigned BFHFNMIGN : 1;
	unsigned STKALIGN : 1;
	unsigned : 22;
} CCR_t;
_Static_assert(sizeof(CCR_t) == 4U, "CCR_t is wrong size");
#define CCR (*(volatile CCR_t *) 0xE000ED14)

typedef struct {
	unsigned PRI_4 : 8;
	unsigned PRI_5 : 8;
	unsigned PRI_6 : 8;
	unsigned : 8;
} SHPR1_t;
_Static_assert(sizeof(SHPR1_t) == 4U, "SHPR1_t is wrong size");
#define SHPR1 (*(volatile SHPR1_t *) 0xE000ED18)

typedef struct {
	unsigned : 24;
	unsigned PRI_11 : 8;
} SHPR2_t;
_Static_assert(sizeof(SHPR2_t) == 4U, "SHPR2_t is wrong size");
#define SHPR2 (*(volatile SHPR2_t *) 0xE000ED1C)

typedef struct {
	unsigned : 16;
	unsigned PRI_14 : 8;
	unsigned PRI_15 : 8;
} SHPR3_t;
_Static_assert(sizeof(SHPR3_t) == 4U, "SHPR3_t is wrong size");
#define SHPR3 (*(volatile SHPR3_t *) 0xE000ED20)

typedef struct {
	unsigned MEMFAULTACT : 1;
	unsigned BUSFAULTACT : 1;
	unsigned : 1;
	unsigned USGFAULTACT : 1;
	unsigned : 3;
	unsigned SVCALLACT : 1;
	unsigned MONITORACT : 1;
	unsigned : 1;
	unsigned PENDSVACT : 1;
	unsigned SYSTICKACT : 1;
	unsigned USGFAULTPENDED : 1;
	unsigned MEMFAULTPENDED : 1;
	unsigned BUSFAULTPENDED : 1;
	unsigned SVCALLPENDED : 1;
	unsigned MEMFAULTENA : 1;
	unsigned BUSFAULTENA : 1;
	unsigned USGFAULTENA : 1;
	unsigned : 13;
} SHCSR_t;
_Static_assert(sizeof(SHCSR_t) == 4U, "SHCSR_t is wrong size");
#define SHCSR (*(volatile SHCSR_t *) 0xE000ED24)

typedef struct {
	unsigned IACCVIOL : 1;
	unsigned DACCVIOL : 1;
	unsigned : 1;
	unsigned MUNSTKERR : 1;
	unsigned MSTKERR : 1;
	unsigned MLSPERR : 1;
	unsigned : 1;
	unsigned MMARVALID : 1;
	unsigned IBUSERR : 1;
	unsigned PRECISERR : 1;
	unsigned IMPRECISERR : 1;
	unsigned UNSTKERR : 1;
	unsigned STKERR : 1;
	unsigned LSPERR : 1;
	unsigned : 1;
	unsigned BFARVALID : 1;
	unsigned UNDEFINSTR : 1;
	unsigned INVSTATE : 1;
	unsigned INVPC : 1;
	unsigned NOCP : 1;
	unsigned : 4;
	unsigned UNALIGNED : 1;
	unsigned DIVBYZERO : 1;
	unsigned : 6;
} CFSR_t;
_Static_assert(sizeof(CFSR_t) == 4U, "CFSR_t is wrong size");
#define CFSR (*(volatile CFSR_t *) 0xE000ED28)

typedef struct {
	unsigned : 1;
	unsigned VECTTBL : 1;
	unsigned : 28;
	unsigned FORCED : 1;
	unsigned DEBUGEVT : 1;
} HFSR_t;
_Static_assert(sizeof(HFSR_t) == 4U, "HFSR_t is wrong size");
#define HFSR (*(volatile HFSR_t *) 0xE000ED2C)

#define MMFAR (*(void * volatile *) 0xE000ED34)
#define BFAR (*(void * volatile *) 0xE000ED38)
#define AFSR (*(volatile uint32_t *) 0xE000ED3C)


#endif

