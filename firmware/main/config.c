#include <pic18fregs.h>

/**
 * \file
 *
 * \brief Sets the configuration fuses.
 */

/*
 *                                                /-------- In-circuit debugging disabled
 *                                                |/------- Extended instruction set and addressing disabled
 *                                                ||/------ Reset on stack overflow/underflow
 *                                                |||/----- Unused
 *                                                ||||///-- Divide by 2 on PLL input, oscillator is 8MHz.
 *                                                |||||||/- Watchdog timer disabled
 *                                                |||||||| */
static __code const char __at(__CONFIG1L) c1l = 0b10101100;

/*
 *                                                /////---- Unused
 *                                                |||||/--- Program memory not protected
 *                                                ||||||//- CPU clock not divided
 *                                                |||||||| */
static __code const char __at(__CONFIG1H) c1h = 0b11110111;

/*
 *                                                /-------- Two speed startup disabled
 *                                                |/------- Fail-safe clock monitor enabled
 *                                                ||///---- Unused
 *                                                |||||///- External clock, PLL enabled, used for CPU and USB
 *                                                |||||||| */
static __code const char __at(__CONFIG2L) c2l = 0b01000111;

/*
 *                                                ////----- Unused
 *                                                ||||////- Watchdog postscaler 1:1
 *                                                |||||||| */
static __code const char __at(__CONFIG2H) c2h = 0b11110000;

/*
 *                                                /-------- Ignored
 *                                                |/------- Ignored
 *                                                ||//----- External memory bus disabled
 *                                                ||||/---- Ignored
 *                                                |||||///- Unused
 *                                                |||||||| */
static __code const char __at(__CONFIG3L) c3l = 0b00110000;

/*
 *                                                ////----- Unused
 *                                                ||||/---- MSSP 7-bit address masking in I²C slave mode
 *                                                |||||//-- Ignored
 *                                                |||||||/- ECCP2 on RC1
 *                                                |||||||| */
static __code const char __at(__CONFIG3H) c3h = 0b11111001;

