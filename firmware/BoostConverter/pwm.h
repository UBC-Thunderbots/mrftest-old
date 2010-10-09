/* PWM generator header file */
/* Generates PWM with a constant on time for CCP1. On Time calculated from theoretical values */
/* Off time varies to match duty cycle input */

#ifndef PWM_H
#define PWM_H

/*PWM initialization Function*/
void InitPWM();

/*Duty cycle control with on time at 17.6E-6s*/
void DCCtrl(float dutyCycle);

#endif
