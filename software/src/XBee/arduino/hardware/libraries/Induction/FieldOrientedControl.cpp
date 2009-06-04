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

#include "FieldOrientedControl.h"
#include "WProgram.h"

void FieldOrientedControl::setFluxRef(float dRef){
	// takes average of current scaling values
	rotorFluxRef = ((iScalarA+iScalarB)/2) * dRef/2;
}

void FieldOrientedControl::runMotor(int iterations)
{
	for(int i=0; i< iterations; i++){

		//clarke transform (includes reading input currents)
		statorCurrentAlpha = iScalarA * (analogRead(phaseACurrentPin) - IN_MAX/2);
		statorCurrentBeta = (1/sqrt(3)) * statorCurrentAlpha + (2/sqrt(3))* (iScalarB * (analogRead(phaseBCurrentPin) - IN_MAX/2));

		//TODO:estimate rotor flux position theta
		//TODO: statorVoltageDlk = 

		//park transform
		statorCurrentD = statorCurrentAlpha * cos(rotorFluxAngle) + statorCurrentBeta * sin(rotorFluxAngle);
		statorCurrentQ = -statorCurrentAlpha * sin(rotorFluxAngle) + statorCurrentBeta * cos(rotorFluxAngle);

		//PID: use inputs rotorFluxRef and torqueRef and get difference with measured values to get final values, 
		rotorFluxErr = rotorFluxRef - statorCurrentD;
		statorVoltageDk = xFlux + KPFlux*rotorFluxErr;
		statorVoltageDlk = max(min(statorVoltageDk, vdc),-vdc);
		xFlux = xFlux + KIFlux * rotorFluxErr + KCorFlux * (statorVoltageDk - statorVoltageDlk);

		torqueErr = torqueRef - statorCurrentQ;
		statorVoltageQk = xTorque + KPTorque*torqueErr;
		statorVoltageQlk = max(min(statorVoltageQk, sqrt(pow(vdc,2) - pow(statorVoltageDlk,2))), -sqrt(pow(vdc,2) - pow(statorVoltageDlk,2)));
		xTorque = xTorque + KITorque * torqueErr + KCorTorque * (statorVoltageQk - statorVoltageQlk);

		//inverse park transform
		statorVoltageAlpha = statorVoltageDlk * cos(rotorFluxAngle) - statorVoltageQlk * sin(rotorFluxAngle);
		statorVoltageBeta = statorVoltageDlk * sin(rotorFluxAngle) + statorVoltageQlk * cos(rotorFluxAngle);
		
		//inverse clarke transform
		statorVOUTA = statorVoltageAlpha*(OUT_MAX/vdc);
		statorVOUTB = ((sqrt(3) * statorVoltageBeta - statorVoltageAlpha)/2) * (OUT_MAX/vdc);
		statorVOUTC = -(statorVOUTA + statorVOUTB);

		//output pwm signals TODO: figure out pwm signals
		analogWrite(pinPhaseALow, (statorVOUTA < 0) * abs(statorVOUTA));
		analogWrite(pinPhaseBLow, (statorVOUTB < 0) * abs(statorVOUTB));
		analogWrite(pinPhaseCLow, (statorVOUTC < 0) * abs(statorVOUTC));
		analogWrite(pinPhaseAHigh, (statorVOUTA > 0) * abs(statorVOUTA));
		analogWrite(pinPhaseBHigh, (statorVOUTB > 0) * abs(statorVOUTB));
		analogWrite(pinPhaseCHigh, (statorVOUTC > 0) * abs(statorVOUTC));

		// update memory variables
		statorCurrentAlphaOld = statorCurrentAlpha;
		statorCurrentBetaOld = statorCurrentBeta;

	}
}

bool FieldOrientedControl::initializeMotor()
{
	//pid variables initialization
	xFlux = 0;
	xTorque = 0;
	KPFlux = .5;
	KIFlux = .5;
	KPTorque = .5;
	KITorque = .5;

	statorCurrentAlpha = 0;
	statorCurrentBeta = 0;	
	statorCurrentAlphaOld = 0;
	statorCurrentBetaOld = 0;

	//starts rotor flux angle at 0 
	rotorFluxAngle = 0;

	//calibration
	float calibrationV = 5;

	pinMode(pinPhaseALow, OUTPUT);
	pinMode(pinPhaseBLow, OUTPUT);
	pinMode(pinPhaseCLow, OUTPUT);
	pinMode(pinPhaseAHigh, OUTPUT);
	pinMode(pinPhaseBHigh, OUTPUT);
	pinMode(pinPhaseCHigh, OUTPUT);

	statorVOUTA = calibrationV * sin(4*PI/3);
	statorVOUTB = calibrationV * sin(2*PI/3);
	statorVOUTC = calibrationV * sin(0);

	analogWrite(pinPhaseALow, (statorVOUTA < 0) * abs(statorVOUTA));
	analogWrite(pinPhaseBLow, (statorVOUTB < 0) * abs(statorVOUTB));
	analogWrite(pinPhaseCLow, (statorVOUTC < 0) * abs(statorVOUTC));
	analogWrite(pinPhaseAHigh, (statorVOUTA > 0) * abs(statorVOUTA));
	analogWrite(pinPhaseBHigh, (statorVOUTB > 0) * abs(statorVOUTB));
	analogWrite(pinPhaseCHigh, (statorVOUTC > 0) * abs(statorVOUTC));


	// allow for solenoids(motor coils) to saturate
	delay(500);

	// calibrate current sensors
	iScalarA = (calibrationV/OUT_MAX)*(vdc/r)/(analogRead(phaseACurrentPin) - IN_MAX/2);
	iScalarB = (calibrationV/OUT_MAX)*(vdc/r)/(analogRead(phaseBCurrentPin) - IN_MAX/2);

	if(iScalarA - iScalarB > .5) return false;

	// rotor flux reference starts at quarter max current
	rotorFluxRef = (IN_MAX/8)*(iScalarA + iScalarB)/2;

	return true;
}
