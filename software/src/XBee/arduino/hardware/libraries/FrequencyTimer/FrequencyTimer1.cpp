/*
  FrequencyTimer1.h - A frequency generator and interrupt generator library
  Author: Jim Studt, jim@federated.com
  Copyright (c) 2007 David A. Mellis.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <FrequencyTimer1.h>

#include <avr/interrupt.h>

void (*FrequencyTimer1::onOverflow)() = 0;
uint8_t FrequencyTimer1::enabled = 0;

#if defined(__AVR_ATmega168__)
SIGNAL(SIG_OUTPUT_COMPARE1A)
#else
SIGNAL(SIG_OUTPUT_COMPARE1)
#endif
{
    static uint8_t inHandler = 0; // protect us from recursion if our handler enables interrupts

    if ( !inHandler && FrequencyTimer1::onOverflow) {
	inHandler = 1;
	(*FrequencyTimer1::onOverflow)();
	inHandler = 0;
    }
}

void FrequencyTimer1::setOnOverflow( void (*func)() )
{
    FrequencyTimer1::onOverflow = func;
#if defined(__AVR_ATmega168__)
    if ( func) TIMSK1 |= _BV(OCIE1A);
    else TIMSK1 &= ~_BV(OCIE1A);
#else
    if ( func) TIMSK |= _BV(OCIE1);
    else TIMSK &= ~_BV(OCIE1);
#endif
}

void FrequencyTimer1::setPeriod(unsigned long period)
{
    uint8_t pre, top;
  
    if ( period == 0) period = 1;
    period *= clockCyclesPerMicrosecond();
 
    period /= 2;            // we work with half-cycles before the toggle 
    if ( period <= 256) {
	pre = 1;
	top = period-1;
    } else if ( period <= 256L*8) {
	pre = 2;
	top = period/8-1;
    } else if ( period <= 256L*32) {
	pre = 3;
	top = period/32-1;
    } else if ( period <= 256L*64) {
	pre = 4;
	top = period/64-1;
    } else if ( period <= 256L*128) {
	pre = 5;
	top = period/128-1;
    } else if ( period <= 256L*256) {
	pre = 6;
	top = period/256-1;
    } else if ( period <= 256L*1024) {
	pre = 7;
	top = period/1024-1;
    } else {
	pre = 7;
	top = 255;
    }

#if defined(__AVR_ATmega168__)
    TCCR1B = 0;
    TCCR1A = 0;
    TCNT1 = 0;
    ASSR &= ~_BV(AS2);    // use clock, not T2 pin
    OCR1A = top;
    TCCR1A = (_BV(WGM11) | ( FrequencyTimer1::enabled ? _BV(COM1A0) : 0));
    TCCR1B = pre;
#else
    TCCR1 = 0;
    TCNT1 = 0;
    ASSR &= ~_BV(AS1);    // use clock, not T2 pin
    OCR1 = top;
    TCCR1 = (_BV(WGM11) | ( FrequencyTimer1::enabled ? _BV(COM10) : 0)  | pre);
#endif
}

unsigned long  FrequencyTimer1::getPeriod()
{
#if defined(__AVR_ATmega168__)
    uint8_t p = (TCCR1B & 7);
    unsigned long v = OCR1A;
#else
    uint8_t p = (TCCR1 & 7);
    unsigned long v = OCR1;
#endif
    uint8_t shift;
  
    switch(p) {
      case 0 ... 1:
	shift = 0;
	break;
      case 2:
	shift = 3;
	break;
      case 3:
	shift = 5;
	break;
      case 4:
	shift = 6;
	break;
      case 5:
	shift = 7;
	break;
      case 6:
	shift = 8;
	break;
      case 7:
	shift = 10;
	break;
    }
    return (((v+1) << (shift+1)) + 1) / clockCyclesPerMicrosecond();   // shift+1 converts from half-period to period
}

void FrequencyTimer1::enable()
{
    FrequencyTimer1::enabled = 1;
#if defined(__AVR_ATmega168__)
    TCCR1A |= _BV(COM1A0);
#else
    TCCR1 |= _BV(COM10);
#endif
}

void FrequencyTimer1::disable()
{
    FrequencyTimer1::enabled = 0;
#if defined(__AVR_ATmega168__)
    TCCR1A &= ~_BV(COM1A0);
#else
    TCCR1 &= ~_BV(COM10);
#endif
}
