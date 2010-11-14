/*-------------------------------------------------------------------------
   signal.h - Signal handler header

   Copyright (C) 2005, Vangelis Rokas <vrokas AT otenet.gr>

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2.1, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License 
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

/* interrupt testing arguments */
#define SIG_RB		SIG_RBIF
#define SIG_INT0	SIG_INT0IF
#define SIG_INT1	SIG_INT1IF
#define SIG_INT2	SIG_INT2IF
#define SIG_INT3	SIG_INT3IF
#define SIG_CCP1	SIG_CCP1IF
#define SIG_CCP2	SIG_CCP2IF
#define SIG_CCP3	SIG_CCP3IF
#define SIG_CCP4	SIG_CCP4IF
#define SIG_CCP5	SIG_CCP5IF
#define SIG_CM1		SIG_CM1IF
#define SIG_CM2		SIG_CM2IF
#define SIG_TMR0	SIG_TMR0IF
#define SIG_TMR1	SIG_TMR1IF
#define SIG_TMR2	SIG_TMR2IF
#define SIG_TMR3	SIG_TMR3IF
#define SIG_TMR4	SIG_TMR4IF
#define SIG_BCL1	SIG_BCL1IF
#define SIG_BCL2	SIG_BCL2IF
#define SIG_LVD		SIG_LVDIF
#define SIG_PMP		SIG_PMPIF
#define SIG_AD		SIG_ADIF
#define SIG_RC1		SIG_RC1IF
#define SIG_TX1		SIG_TX1IF
#define SIG_RC2		SIG_RC2IF
#define SIG_TX2		SIG_TX2IF
#define SIG_SSP1	SIG_SSP1IF
#define SIG_MSSP1	SIG_SSP1IF	/* just an alias */
#define SIG_SSP2	SIG_SSP2IF
#define SIG_MSSP2	SIG_SSP2IF	/* just an alias */
#define SIG_OSCF	SIG_OSCFIF
#define SIG_USB		SIG_USBIF

/* define name to be the interrupt handler for interrupt #vecno */
#define DEF_ABSVECTOR(vecno, name)                      \
void __ivt_ ## name(void) __interrupt(vecno) __naked    \
{                                                       \
  __asm goto _ ## name __endasm;                        \
}

/* Define name to be the handler for high priority interrupts,
 * use like this:
 *   DEF_INTHIGH(high_handler)
 *     DEF_HANDLER(SIG_TMR0, timer0_handler)
 *     DEF_HANDLER2(SIG_TMR1, SIG_TMR1IE, timer1_handler)
 *     ...
 *   END_DEF
 *
 *   SIGHANDLER(timer0_handler)
 *   {
 *     // code to handle timer0 interrupts
 *   }
 *   SIGHANDLER(timer1_handler)
 *   {
 *     // code to handle timer1 interrupts
 *   }
 */
#define DEF_INTHIGH(name)                \
DEF_ABSVECTOR(1, name)                   \
void name(void) __naked __interrupt      \
{
  
/* Define name to be the handler for high priority interrupts,
 * use like this:
 *   DEF_INTLOW(low_handler)
 *     DEF_HANDLER(SIG_RB, portb_handler)
 *     DEF_HANDLER2(SIG_LVD, SIG_LVDIE, lowvolt_handler)
 *     ...
 *   END_DEF
 *
 *   SIGHANDLER(portb_handler)
 *   {
 *     // code to handle PORTB change interrupts
 *   }
 *   SIGHANDLER(lowvolt_handler)
 *   {
 *     // code to handle low voltage interrupts
 *   }
 */
#define DEF_INTLOW(name)                 \
DEF_ABSVECTOR(2, name)                   \
void name(void) __naked __interrupt      \
{

/* finish an interrupt handler definition */
#define END_DEF                                 \
  __asm retfie __endasm;                        \
}

/* Declare handler to be the handler function for the given signal.
 * sig should be one of SIG_xxx from above, handler should be a
 * function defined using SIGHANDLER(handler) or
 * SIGHANDLERNAKED(handler).
 * ATTENTION: This macro ignores the signal's enable bit!
 *            Use DEF_HANDLER2(SIG_xxx, SIGxxxIE, handler) instead!
 * To be used together with DEF_INTHIGH and DEF_INTLOW.
 */
#define DEF_HANDLER(sig, handler)               \
    __asm btfsc sig __endasm;                   \
    __asm goto  _ ## handler __endasm;

/* Declare handler to be the handler function for the given signal.
 * sig should be one of SIG_xxx from above,
 * sig2 should also be a signal (probably SIG_xxxIE from below) and
 * handler should be a function defined using SIGHANDLER(handler)
 * or SIGHANDLERNAKED(handler).
 * To be used together with DEF_INTHIGH and DEF_INTLOW.
 */
#define DEF_HANDLER2(sig1,sig2,handler)         \
    __asm btfss sig1 __endasm;                  \
    __asm bra   $+8 __endasm;                   \
    __asm btfsc sig2 __endasm;                  \
    __asm goto  _ ## handler __endasm;

/* Declare or define an interrupt handler function. */
#define SIGHANDLER(handler)             void handler (void) __interrupt
#define SIGHANDLERNAKED(handler)        void handler (void) __naked __interrupt


/*
 * inline assembly compatible bit definitions
 */
#define SIG_RBIF	_INTCON, 0
#define SIG_RBIE	_INTCON, 3
#define SIG_RBIP	_INTCON2, 0

#define SIG_INT0IF	_INTCON, 1
#define SIG_INT0IE	_INTCON, 4
/*#define SIG_INT0IP not selectable, always ? */

#define SIG_TMR0IF	_INTCON, 2
#define SIG_TMR0IE	_INTCON, 5
#define SIG_TMR0IP	_INTCON2, 2

#define SIG_INT1IF	_INTCON3, 0
#define SIG_INT1IE	_INTCON3, 3
#define SIG_INT1IP	_INTCON3, 6

#define SIG_INT2IF	_INTCON3, 1
#define SIG_INT2IE	_INTCON3, 4
#define SIG_INT2IP	_INTCON3, 7

#define SIG_INT3IF	_INTCON3, 2
#define SIG_INT3IE	_INTCON3, 5
#define SIG_INT3IP	_INTCON2, 1

/* device dependent -- should be moved to pic18f*.h */
#define SIG_TMR1IDX	0
#define SIG_TMR1SUF	1
#define SIG_TMR2IDX	1
#define SIG_TMR2SUF	1
#define SIG_CCP1IDX	2
#define SIG_CCP1SUF	1
#define SIG_SSP1IDX	3
#define SIG_SSP1SUF	1
#define SIG_TX1IDX	4
#define SIG_TX1SUF	1
#define SIG_RC1IDX	5
#define SIG_RC1SUF	1
#define SIG_ADIDX	6
#define SIG_ADSUF	1
#define SIG_PMPIDX	7
#define SIG_PMPSUF	1

#define SIG_CCP2IDX	0
#define SIG_CCP2SUF	2
#define SIG_TMR3IDX	1
#define SIG_TMR3SUF	2
#define SIG_LVDIDX	2
#define SIG_LVDSUF	2
#define SIG_BCL1IDX	3
#define SIG_BCL1SUF	2
#define SIG_USBIDX	4
#define SIG_USBSUF	2
#define SIG_CM1IDX	5
#define SIG_CM1SUF	2
#define SIG_CM2IDX	6
#define SIG_CM2SUF	2
#define SIG_OSCFIDX	7
#define SIG_OSCFSUF	2

#define SIG_CCP3IDX	0
#define SIG_CCP3SUF	3
#define SIG_CCP4IDX	1
#define SIG_CCP4SUF	3
#define SIG_CCP5IDX	2
#define SIG_CCP5SUF	3
#define SIG_TMR4IDX	3
#define SIG_TMR4SUF	3
#define SIG_TX2IDX	4
#define SIG_TX2SUF	3
#define SIG_RC2IDX	5
#define SIG_RC2SUF	3
#define SIG_BCL2IDX	6
#define SIG_BCL2SUF	3
#define SIG_SSP2IDX	7
#define SIG_SSP2SUF	3

/* device independent */
#define __concat(a,b)   __concat2(a,b)
#define __concat2(a,b)  a ## b

#define SIG_PIR(suf)	__concat(_PIR,suf)
#define SIG_PIE(suf)	__concat(_PIE,suf)
#define SIG_IPR(suf)	__concat(_IPR,suf)

#define SIG_TMR1IF	SIG_PIR(SIG_TMR1SUF), SIG_TMR1IDX
#define SIG_TMR1IE	SIG_PIE(SIG_TMR1SUF), SIG_TMR1IDX
#define SIG_TMR1IP	SIG_IPR(SIG_TMR1SUF), SIG_TMR1IDX

#define SIG_TMR2IF	SIG_PIR(SIG_TMR2SUF), SIG_TMR2IDX
#define SIG_TMR2IE	SIG_PIE(SIG_TMR2SUF), SIG_TMR2IDX
#define SIG_TMR2IP	SIG_IPR(SIG_TMR2SUF), SIG_TMR2IDX

#define SIG_CCP1IF	SIG_PIR(SIG_CCP1SUF), SIG_CCP1IDX
#define SIG_CCP1IE	SIG_PIE(SIG_CCP1SUF), SIG_CCP1IDX
#define SIG_CCP1IP	SIG_IPR(SIG_CCP1SUF), SIG_CCP1IDX

#define SIG_SSP1IF	SIG_PIR(SIG_SSP1SUF), SIG_SSP1IDX
#define SIG_SSP1IE	SIG_PIE(SIG_SSP1SUF), SIG_SSP1IDX
#define SIG_SSP1IP	SIG_IPR(SIG_SSP1SUF), SIG_SSP1IDX
/* aliases: MSSP1 */
#define SIG_MSSP1IF	SIG_SSP1IF	//SIG_PIR(SIG_SSP1SUF), SIG_SSP1IDX
#define SIG_MSSP1IE	SIG_SSP1IE	//SIG_PIE(SIG_SSP1SUF), SIG_SSP1IDX
#define SIG_MSSP1IP	SIG_SSP1IP	//SIG_IPR(SIG_SSP1SUF), SIG_SSP1IDX

#define SIG_TX1IF	SIG_PIR(SIG_TX1SUF), SIG_TX1IDX
#define SIG_TX1IE	SIG_PIE(SIG_TX1SUF), SIG_TX1IDX
#define SIG_TX1IP	SIG_IPR(SIG_TX1SUF), SIG_TX1IDX

#define SIG_RC1IF	SIG_PIR(SIG_RC1SUF), SIG_RC1IDX
#define SIG_RC1IE	SIG_PIE(SIG_RC1SUF), SIG_RC1IDX
#define SIG_RC1IP	SIG_IPR(SIG_RC1SUF), SIG_RC1IDX

#define SIG_ADIF	SIG_PIR(SIG_ADSUF), SIG_ADIDX
#define SIG_ADIE	SIG_PIE(SIG_ADSUF), SIG_ADIDX
#define SIG_ADIP	SIG_IPR(SIG_ADSUF), SIG_ADIDX

#define SIG_PMPIF	SIG_PIR(SIG_PMPSUF), SIG_PMPIDX
#define SIG_PMPIE	SIG_PIE(SIG_PMPSUF), SIG_PMPIDX
#define SIG_PMPIP	SIG_IPR(SIG_PMPSUF), SIG_PMPIDX

#define SIG_CCP2IF	SIG_PIR(SIG_CCP2SUF), SIG_CCP2IDX
#define SIG_CCP2IE	SIG_PIE(SIG_CCP2SUF), SIG_CCP2IDX
#define SIG_CCP2IP	SIG_IPR(SIG_CCP2SUF), SIG_CCP2IDX

#define SIG_TMR3IF	SIG_PIR(SIG_TMR3SUF), SIG_TMR3IDX
#define SIG_TMR3IE	SIG_PIE(SIG_TMR3SUF), SIG_TMR3IDX
#define SIG_TMR3IP	SIG_IPR(SIG_TMR3SUF), SIG_TMR3IDX

#define SIG_LVDIF	SIG_PIR(SIG_LVDSUF), SIG_LVDIDX
#define SIG_LVDIE	SIG_PIE(SIG_LVDSUF), SIG_LVDIDX
#define SIG_LVDIP	SIG_IPR(SIG_LVDSUF), SIG_LVDIDX

#define SIG_BCL1IF	SIG_PIR(SIG_BCL1SUF), SIG_BCL1IDX
#define SIG_BCL1IE	SIG_PIE(SIG_BCL1SUF), SIG_BCL1IDX
#define SIG_BCL1IP	SIG_IPR(SIG_BCL1SUF), SIG_BCL1IDX

#define SIG_USBIF	SIG_PIR(SIG_USBSUF), SIG_USBIDX
#define SIG_USBIE	SIG_PIE(SIG_USBSUF), SIG_USBIDX
#define SIG_USBIP	SIG_IPR(SIG_USBSUF), SIG_USBIDX

#define SIG_CM1IF	SIG_PIR(SIG_CM1SUF), SIG_CM1IDX
#define SIG_CM1IE	SIG_PIE(SIG_CM1SUF), SIG_CM1IDX
#define SIG_CM1IP	SIG_IPR(SIG_CM1SUF), SIG_CM1IDX

#define SIG_CM2IF	SIG_PIR(SIG_CM2SUF), SIG_CM2IDX
#define SIG_CM2IE	SIG_PIE(SIG_CM2SUF), SIG_CM2IDX
#define SIG_CM2IP	SIG_IPR(SIG_CM2SUF), SIG_CM2IDX

#define SIG_OSCFIF	SIG_PIR(SIG_OSCFSUF), SIG_OSCFIDX
#define SIG_OSCFIE	SIG_PIE(SIG_OSCFSUF), SIG_OSCFIDX
#define SIG_OSCFIP	SIG_IPR(SIG_OSCFSUF), SIG_OSCFIDX

#define SIG_CCP3IF	SIG_PIR(SIG_CCP3SUF), SIG_CCP3IDX
#define SIG_CCP3IE	SIG_PIE(SIG_CCP3SUF), SIG_CCP3IDX
#define SIG_CCP3IP	SIG_IPR(SIG_CCP3SUF), SIG_CCP3IDX

#define SIG_CCP4IF	SIG_PIR(SIG_CCP4SUF), SIG_CCP4IDX
#define SIG_CCP4IE	SIG_PIE(SIG_CCP4SUF), SIG_CCP4IDX
#define SIG_CCP4IP	SIG_IPR(SIG_CCP4SUF), SIG_CCP4IDX

#define SIG_CCP5IF	SIG_PIR(SIG_CCP5SUF), SIG_CCP5IDX
#define SIG_CCP5IE	SIG_PIE(SIG_CCP5SUF), SIG_CCP5IDX
#define SIG_CCP5IP	SIG_IPR(SIG_CCP5SUF), SIG_CCP5IDX

#define SIG_TMR4IF	SIG_PIR(SIG_TMR4SUF), SIG_TMR4IDX
#define SIG_TMR4IE	SIG_PIE(SIG_TMR4SUF), SIG_TMR4IDX
#define SIG_TMR4IP	SIG_IPR(SIG_TMR4SUF), SIG_TMR4IDX

#define SIG_TX2IF	SIG_PIR(SIG_TX2SUF), SIG_TX2IDX
#define SIG_TX2IE	SIG_PIE(SIG_TX2SUF), SIG_TX2IDX
#define SIG_TX2IP	SIG_IPR(SIG_TX2SUF), SIG_TX2IDX

#define SIG_RC2IF	SIG_PIR(SIG_RC2SUF), SIG_RC2IDX
#define SIG_RC2IE	SIG_PIE(SIG_RC2SUF), SIG_RC2IDX
#define SIG_RC2IP	SIG_IPR(SIG_RC2SUF), SIG_RC2IDX

#define SIG_BCL2IF	SIG_PIR(SIG_BCL2SUF), SIG_BCL2IDX
#define SIG_BCL2IE	SIG_PIE(SIG_BCL2SUF), SIG_BCL2IDX
#define SIG_BCL2IP	SIG_IPR(SIG_BCL2SUF), SIG_BCL2IDX

#define SIG_SSP2IF	SIG_PIR(SIG_SSP2SUF), SIG_SSP2IDX
#define SIG_SSP2IE	SIG_PIE(SIG_SSP2SUF), SIG_SSP2IDX
#define SIG_SSP2IP	SIG_IPR(SIG_SSP2SUF), SIG_SSP2IDX

#endif  /* __SIGNAL_H__ */
