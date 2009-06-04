#ifndef FOC_3_PHASE
#define FOC_3_PHASE

/*
  FOC 3 Phase induction motor controller.
  Author: George Stelle, stelleg@gmail.com
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


class FieldOrientedControl
{
	private:
		// R is resistance for any given phase.  Note that if there are n poles 1/r = 1/r1 + 1/r2 ... + 1/rn
		float r;
		// rDiv is a constant derived to multiply the input dc voltage to get the real dc voltage. it should be r2/r1.
		float rDiv;		
		// to store the real dc voltage. 
		float vdc;	
		// current sensor calibration values using r and v, such that i = iScalar*(iref-512)
		float iScalarA;
		float iScalarB;

		//pin declarations
		static const int OUT_MAX = 255;
		static const int IN_MAX = 1023;
		
		//output pwm pins
		static const int pinPhaseALow = 3;
		static const int pinPhaseBLow = 5;
		static const int pinPhaseCLow = 6;
		static const int pinPhaseAHigh = 9;
		static const int pinPhaseBHigh = 10;
		static const int pinPhaseCHigh = 11;
		
		
		//current input pins
		static const int phaseACurrentPin = 0;
		static const int phaseBCurrentPin = 1;		
		
		//input (torqueRef)
		static const int torqueRefPin = 2;
	
		// all angles relative to alpha(= phase a) in radians.
		float rotorFluxAngle;

		// torqueRef will be input via potentiometer, while fluxRef should be set constant. (can play with values to find optimal value)
		// note also that stator current magnitude = sqrt(torqueRef^2 + fluxRef^2)
		float torqueRef;
		float rotorFluxRef;
		
		//pid variables
		float rotorFluxErr;
		float torqueErr;
		float xFlux;
		float xTorque;
		float KPFlux;
		float KIFlux;
		float KCorFlux;
		float KPTorque;
		float KITorque;
		float KCorTorque;		
		float statorVoltageDk;
		float statorVoltageQk;

		// output voltages in d,q reference frame; input to inverse park transform, output from PID
		float statorVoltageDlk;
		float statorVoltageQlk;

		float statorVOUTA;
		float statorVOUTB;
		float statorVOUTC;		

		// ouput voltages from inverse park transform, input to SVM
		float statorVoltageAlpha;
		float statorVoltageBeta;

		// clarke transform variables
		float statorCurrentAlpha;
		float statorCurrentBeta;

		// saved values for rotor flux estimation
		float statorCurrentAlphaOld;
		float statorCurrentBetaOld;

		// park transform variables
		float statorCurrentD;
		float statorCurrentQ;

	public:
		bool initializeMotor();
		void runMotor(int iterations);		
		void setFluxRef(float dRef);
};
	


#endif
