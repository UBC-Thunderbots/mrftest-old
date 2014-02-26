#ifndef STM32LIB_REGISTERS_SYSTICK_H
#define STM32LIB_REGISTERS_SYSTICK_H

/**
 * \file
 *
 * \brief Defines the system timer registers.
 */

typedef struct {
	unsigned ENABLE : 1;
	unsigned TICKINT : 1;
	unsigned CLKSOURCE : 1;
	unsigned : 13;
	unsigned COUNTFLAG : 1;
	unsigned : 15;
} SYST_CSR_t;
_Static_assert(sizeof(SYST_CSR_t) == 4U, "SYST_CSR_t is wrong size");
#define SYST_CSR (*(volatile SYST_CSR_t *) (0xE000E010))

typedef struct {
	unsigned RELOAD : 24;
	unsigned : 8;
} SYST_RVR_t;
_Static_assert(sizeof(SYST_RVR_t) == 4U, "SYST_RVR_t is wrong size");
#define SYST_RVR (*(volatile SYST_RVR_t *) (0xE000E014))

typedef struct {
	unsigned CURRENT : 24;
	unsigned : 8;
} SYST_CVR_t;
_Static_assert(sizeof(SYST_CVR_t) == 4U, "SYST_CVR_t is wrong size");
#define SYST_CVR (*(volatile SYST_CVR_t *) (0xE000E018))

typedef struct {
	unsigned TENMS : 24;
	unsigned : 6;
	unsigned SKEW : 1;
	unsigned NOREF : 1;
} SYST_CALIB_t;
_Static_assert(sizeof(SYST_CALIB_t) == 4U, "SYST_CALIB_t is wrong size");
#define SYST_CALIB (*(volatile SYST_CALIB_t *) (0xE000E01C))

#endif

