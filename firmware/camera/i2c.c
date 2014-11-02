
#include "i2c.h"
#include "rcc.h"
#include <registers/i2c.h>
#include <stdio.h>

bool i2c_init(void) {

	// RCC
	rcc_enable_reset(APB1, I2C);
	
	// Set up peripheral input clock frequency to 40 MHz so that I2C and run at 400kHz (Fast mode)
	I2C_CR2_t cr2 = {
						.ITERREN = 1,
						.FREQ = 40, 
					};
	I2C.CR2 = cr2;

	// Configure clock control registers
	I2C_CCR_t ccr = {
						.FS = 1,		// Fast mode (400kHz)
						.DUTY = 1,		// Duty cycle: 16:9. (In order to reach 400kHz)
						// 400kHz -> 2.5us periods. 1600 ns : 900 ns. 
						// T_high = T_pclk*CCR*9 = 900 ns
						// T_pclk = 25ns (40MHz)
						.CCR = 4,		
					};
	I2C.CCR = ccr;

	// Configure the rise time register
	I2C_TRISE_t trise = {
						// SCCB's max rise time is 300 ns.
						// 300/25ns + 1= 13
						. TRISE = 13,
						};
	I2C.TRISE = trise;

	I2C.OAR1 = {
				.high = 1
				};
	


	// Program the CR1 to enable the peripheral and set start bit
	I2C_CR1_t cr1 = {
						.PE = 1,
						.START = 1,
					};
	I2C.CR1 = cr1;


	//
