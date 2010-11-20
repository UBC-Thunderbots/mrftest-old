#include <pic18f4550.h>
#include "config.h"

#define I_MAX 10.0
#define T_STEP 83.3e-9
#define INDUCTANCE 2.2e-6
#define VIN 14.4
#define VOLTAGE_MAX 100.0
#define DIODE_VOLTAGE 0.7

static void pulse_train(unsigned int off_time_instrs) {
	static unsigned int jump_offset;
	static unsigned char jump_offset_high, jump_offset_low;
	static unsigned char loop_counter;

	/* Clamp argument to legal range. */
	if (off_time_instrs > 2000) {
		off_time_instrs = 2000;
	}
	if (off_time_instrs < 6) {
		off_time_instrs = 6;
	}

	/* We have an overhead of five instructions in the loop header; subtract them from the requested instruction count. */
	off_time_instrs -= 6;

	/* Smaller off times need to jump further into the sled to execute fewer instructions, so negate. */
	jump_offset = 2000 - off_time_instrs;

	/* Each instruction takes 2 cycles (it's a branch), but it also takes 2 bytes. Make sure the offset is even. */
	jump_offset &= 0xFFFE;

	/* Extract high and low parts. */
	jump_offset_high = off_time_instrs >> 8;
	jump_offset_low = off_time_instrs & 0xFF;

	/* We will generate 100 pulses (need an offset of 1 due to loop structure). */
	loop_counter = 101;

	/* Do the magic. */
	_asm
		; Preparation, load the jump target into PCLATU:PCLATH:WREG.
		; Add on the address of the label as we go.
		clrf _PCLATU

		banksel _pulse_train_jump_offset_high_1_1
		movf _pulse_train_jump_offset_high_1_1, W
		movwf _PCLATH

		banksel _pulse_train_jump_offset_low_1_1
		movf _pulse_train_jump_offset_low_1_1, W
		addlw LOW(pulse_train_off_time_sled)
		movwf _pulse_train_jump_offset_low_1_1

		movlw HIGH(pulse_train_off_time_sled)
		addwfc _PCLATH, F

		movf _pulse_train_jump_offset_low_1_1, W

		; We need BSR to point at the loop counter.
		banksel _pulse_train_loop_counter_1_1

pulse_train_enter_loop:
		; Turn off the output.
		bcf _LATC, 2

		; Check if it is time to exit the loop.
		dcfsnz _pulse_train_loop_counter_1_1, F
		goto pulse_train_done

		; Jump into the sled.
		movwf _PCL

		; The sled itself is a thousand BRA $+2 instructions. Each takes 2 instruction cycles and 2 bytes.
		; Because the preprocessor does not emit newlines, we must make each of these a separate assembly block.
		; Thus we drop back into plain C mode.
pulse_train_off_time_sled:
	_endasm;
#define SLED1 _asm bra $+2 _endasm;
#define SLED10 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1 \
	SLED1
#define SLED100 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10 \
	SLED10

	SLED100
	SLED100
	SLED100
	SLED100
	SLED100
	SLED100
	SLED100
	SLED100
	SLED100
	SLED100

	_asm
		; Turn on the output.
		bsf _LATC, 2

		; The on time is 10 instruction cycles. Each BRA takes 2 instruction cycles.
		; The earlier BRA instructions jump to the instruction immediately following.
		; The last intruction (GOTO) jumps back to the top of the loop, and also takes 2 cycles.
		; That last instruction is included in the cycle count.
		bra $+2
		bra $+2
		bra $+2
		bra $+2
		goto pulse_train_enter_loop

pulse_train_done:
	_endasm;
}

void main(void) {

	/* Standard C, declare all variables at the start of the function!*/

	unsigned char i;
	unsigned int reading;
	unsigned int offtime;
	float voltage;
/* PIN Setup */

	INTCON2bits.RBPU = 0; // Internal pullups for port B enabled
	TRISBbits.TRISB0 = 1;//RB0 set to input
	TRISBbits.TRISB1 = 1;//RB1 set to input
	TRISBbits.TRISB2 = 0;//RB2 set to output
	TRISBbits.TRISB3 = 0;//RB3 set to output
	LATB = 0x00;

	/* ADC Setup */

	ADCON0= 0x11;
	TRISA = 0x3F;
	TRISE = 0x01;
	ADCON1 = 0x09;
	
	ADCON2bits.ADFM = 1;//Right Justified
	//Now i need to set conversion time and acquisition times.
	ADCON2bits.ADCS2 = 1;//Tad = 64Tosc;
	ADCON2bits.ADCS1 = 1;//
	ADCON2bits.ADCS0 = 0;//
	ADCON2bits.ACQT2 = 0;//Tacq = 2Tad;
	ADCON2bits.ACQT1 = 0;//
	ADCON2bits.ACQT0 = 1;//
	ADCON0bits.ADON = 1;//Turning on ADC
	
	TRISCbits.TRISC2 = 0;	
	
	//I spin right round baby
	for(;;) {
		
		// Reading Voltage for great justice
		ADCON0bits.GO = 1;
		while( ADCON0bits.GO == 1 ){}
		reading = ADRESH;
		reading = (reading<<8) + ADRESL;
		voltage = reading * 5.0 / 1023.0 * ( 220.0 + 2.2) / 2.2;
	
		// if not charged
		if(voltage < VOLTAGE_MAX){

			//charge done light off
			LATBbits.LATB3 = 0;

			//if charge button pressed
			if(!PORTBbits.RB0) {
				
				//turn on charging light
				LATBbits.LATB2 = 1;

				for(i = 0; i< 10; i++) {
					// mosfet on
					LATCbits.LATC2 = 1;
					_asm
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
						nop;
					_endasm;
					LATCbits.LATC2 = 0;
					
					//this off time calc should move and doesn't do anything now
					offtime = VIN * 10 / (voltage + DIODE_VOLTAGE) * 11;

					//embedded assembly for off time should go here
				}
			}	else {
				//if charge button not pressed leave charge light off
				LATBbits.LATB2 = 0;
			}
		} else {
			// if over voltage threshold turn done light on
			LATBbits.LATB3 = 1;
		}
	}
}

