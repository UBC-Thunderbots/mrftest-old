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
#define SYST_CSR (*(volatile SYST_CSR_t *) (0xE000E010))

typedef struct {
	unsigned RELOAD : 24;
	unsigned : 8;
} SYST_RVR_t;
#define SYST_RVR (*(volatile SYST_RVR_t *) (0xE000E014))

typedef struct {
	unsigned CURRENT : 24;
	unsigned : 8;
} SYST_CVR_t;
#define SYST_CVR (*(volatile SYST_CVR_t *) (0xE000E018))

typedef struct {
	unsigned TENMS : 24;
	unsigned : 6;
	unsigned SKEW : 1;
	unsigned NOREF : 1;
} SYST_CALIB_t;
#define SYST_CALIB (*(volatile SYST_CALIB_t *) (0xE000E01C))

#endif

