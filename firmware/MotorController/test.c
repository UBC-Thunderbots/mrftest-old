#include <pic18f4550.h>

#define BASE 0x66

#define TOGGLE_3P (1 << 7)
#define TOGGLE_3N (1 << 6)
#define TOGGLE_2P (1 << 5)
#deifne TOGGLE_2N (1 << 4)
#define TOGGLE_1P (1 << 3)
#define TOGGLE_1N (1 << 2)

char drive_table_forward[8] = 
{
	// RD7 = 3P, RD6 = 3N, RD5 = 2P, RD4 = 2N, RD3 = 1P, RD2 = 1N
	
	// 000 (shouldn't happen)
	BASE,
	
	// 001
	BASE ^ TOGGLE_1P ^ TOGGLE_3N,
	
	// 010
	BASE ^ TOGGLE_1N ^ TOGGLE_2P,
	
	// 011
	BASE ^ TOGGLE_2P ^ TOGGLE_3N,
	
	// 100
	BASE ^ TOGGLE_3P ^ TOGGLE_2N,
	
	// 101
	BASE ^ TOGGLE_1P ^ TOGGLE_2N,
	
	// 110
	BASE ^ TOGGLE_3P ^ TOGGLE_1N,
	
	// 111 (shouldn't happen)
	BASE
};

void main(void)
{
	volatile int t = 0;
	volatile int r = 0;
	
	unsigned char hall_state;

	TRISB = 1;
	
	TRISD = 0;
	
	while (1)
	{
		hall_state = PORTB >> 2; // LSB = 3, MSB = 1
		
		LATD = drive_table_forward[hall_state];
	}
}

