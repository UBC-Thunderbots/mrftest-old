/**
 * \ingroup REG
 * \defgroup REGSYSTICK System timer
 * @{
 */
#ifndef STM32LIB_REGISTERS_SYSTICK_H
#define STM32LIB_REGISTERS_SYSTICK_H

typedef struct {
	unsigned ENABLE : 1;
	unsigned TICKINT : 1;
	unsigned CLKSOURCE : 1;
	unsigned : 13;
	unsigned COUNTFLAG : 1;
	unsigned : 15;
} SYST_CSR_t;
_Static_assert(sizeof(SYST_CSR_t) == 4U, "SYST_CSR_t is wrong size");

typedef struct {
	unsigned RELOAD : 24;
	unsigned : 8;
} SYST_RVR_t;
_Static_assert(sizeof(SYST_RVR_t) == 4U, "SYST_RVR_t is wrong size");

typedef struct {
	unsigned CURRENT : 24;
	unsigned : 8;
} SYST_CVR_t;
_Static_assert(sizeof(SYST_CVR_t) == 4U, "SYST_CVR_t is wrong size");

typedef struct {
	unsigned TENMS : 24;
	unsigned : 6;
	unsigned SKEW : 1;
	unsigned NOREF : 1;
} SYST_CALIB_t;
_Static_assert(sizeof(SYST_CALIB_t) == 4U, "SYST_CALIB_t is wrong size");

typedef struct {
	SYST_CSR_t CSR;
	SYST_RVR_t RVR;
	SYST_CVR_t CVR;
	SYST_CALIB_t CALIB;
} SYSTICK_t;
_Static_assert(sizeof(SYSTICK_t) == 16U, "SYSTICK_t is wrong size");

extern volatile SYSTICK_t SYSTICK;

#endif

/**
 * @}
 */

