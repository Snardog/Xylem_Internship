#include "stdafx.h"
#include <iostream>
#include <cmath>
#include "polyfit.h"
#include "PEI.h"
#include "spline.h"
#include <vector>
#include "IniReader.h"
#include "ini.h"
#include "database.h"


using namespace std;																	// [VARIABLE DEFINITIONS]
																						
double alpha[3] = { .947, 1, .985 };													// alpha:			stores the alpha values used to calculate PERstd
																						// poleIndex:		a reference to the location of the motor efficiency arrays
int poleIndex, nomSpeed;																// nomSpeed:		the nominal speed for the pump

double q25, q40, q50, q60, q65, q70, q75, q80, q90, q110, q120;							// qXX:				the flow at a certain load point
double h25, h40, h50, h60, h65, h70, h75, h80, h90, hBEP, h110, h120;					// hXX:				the head at a certain load point
double e25, e40, e50, e60, e65, e70, e75, e80, e90, e110, e120;							// eXX:				the efficiency at a certain load point
double pin25, pin60, pin70, pin75, pin80, pin90, pinBEP, pin110, pin120;				// pinXX:			in power into the pump at a certain load point
double pip25, pip60, pip65, pip75, pip90, pipBEP, pip110, pip120;						// pipXX:			the Pump Input Power at a certain load point
double pip25vl, pip50vl, pip75vl;														// pipXXvl:			the Pump Input Power at a certain load point used for variable load calculations
double PERcl, PERstd, PERvl, PERvlLoop, Ns, effPumpSTD, yi;								// PER:				the Pump Efficiency Rating for constant load (cl), variable load (vl), or standard (std)		
double Li, Lfull, PERstdLoop, PERclLoop, temp65, temp90, temp75, tempBEP, temp110;		// PERxxLoop:		the PER value for one load point of the PEI calculation
double Pistd[3], p[3], p_vl[4], y[3], tempCoeff[2], dip[3], cip[4];						// Ns:				specific speed
double PEIcl, PEIvl, PEIcl_spline, PEIvl_spline;										// effPumpSTD:		the standard minimally compliant pump efficiency, used to calculate PERstd
																						// yi:				the part load loss factor at a load point	
motor result;																			// Li:				the part load motor losses at a load point
																						// Lfull:			the motor losses at full load
extern vector<double> hpArray;															// tempXX:			used to store pipXX/motorHP for use in the yi calculation
extern vector<double> effArray2;														// Pistd[3]:		the input power used to calculate PERstd
extern vector<double> effArray4;														// p[3]:			the input powers used to calculate PERcl
extern int stages;																		// p_vl[4]:			the input powers used to calcylate PERvl
																						// y[3]:			same as yi
																						// tempCoeff[2]:	the coefficients used to get pipXX
//Calculates PEIcl																		// dip[3]:			the driver input power, only used to put data into the HI template
//	Uses loading points of 75%, 100%, and 110% BEP flow (or 65%, 90%, 100%)				// cip[4]:			the control input power, only used to put data into the HI template
double getPEIcl(double* eCoeff, double* hCoeff, double qBEP,							// result:			a struct that contains the motor horsepower and efficiency
				double eBEP, bool runOut,
				int speed, double C) {
	//initData(hCoeff, eCoeff, qBEP, eBEP, speed, runOut);

	/*===========================================================================
		Use load points of 60%, 75%, 90%, 100%, 110%, and 120% of BEP flow
		and input power. Preform a linear regression to determine the 
		input power at the required load points.
	
		If the pump can not be tested to 120%, use the loading points of
		60%, 70%, 80%, 90%, 100% of BEP flow
	============================================================================*/

	if (runOut) {
		double pArray[] = { pin60, pin70, pin80, pin90, pinBEP };
		double qArray[] = { q60, q70, q80, q90, qBEP };
		int result = polyfit(qArray, pArray, 5, 1, tempCoeff);
	}
	else {
		double pArray[] = { pin60, pin75, pin90, pinBEP, pin110, pin120 };
		double qArray[] = { q60, q75, q90, qBEP, q110, q120 };
		int result = polyfit(qArray, pArray, 6, 1, tempCoeff);
	}

	//tempCoeff[1] and [0] contain the coefficients from the linear regression
	pip60 = tempCoeff[1] * q60 + tempCoeff[0];
	pip65 = tempCoeff[1] * q65 + tempCoeff[0];
	pip75 = tempCoeff[1] * q75 + tempCoeff[0];
	pip90 = tempCoeff[1] * q90 + tempCoeff[0];
	pipBEP = tempCoeff[1] * qBEP + tempCoeff[0];
	pip110 = tempCoeff[1] * q110 + tempCoeff[0];
	pip120 = tempCoeff[1] * q120 + tempCoeff[0];

	//cout << stages << endl;
	motor result = getMotorHP(pip120);
	Ns = (nomSpeed * pow(qBEP, 0.5)) / pow((hBEP / stages), .75);
	effPumpSTD = -0.85*pow(log(qBEP), 2) - 0.38*log(Ns)*log(qBEP)
		- 11.48*pow(log(Ns), 2) + 17.80*log(qBEP) + 179.80*log(Ns) - (C + 555.6);

	
	//Put all the values needed for the PEI calculation into their seats
	if (runOut) {
		Pistd[0] = ((q65*h65) / 3956) / (alpha[0] * (effPumpSTD / 100));
		Pistd[1] = ((q90*h90) / 3956) / (alpha[1] * (effPumpSTD / 100));
		Pistd[2] = ((qBEP*hBEP) / 3956) / (alpha[2] * (effPumpSTD / 100));

		p[0] = pip65;
		p[1] = pip90;
		p[2] = pipBEP;

		temp65 = pip65 / result.motorHP;
		temp90 = pip90 / result.motorHP;
		tempBEP = pipBEP / result.motorHP;

		y[0] = -0.4508*pow(temp65, 3) + 1.2399*pow(temp65, 2) - 0.4301*temp65 + 0.641;
		y[1] = -0.4508*pow(temp90, 3) + 1.2399*pow(temp90, 2) - 0.4301*temp90 + 0.641;
		y[2] = -0.4508*pow(tempBEP, 3) + 1.2399*pow(tempBEP, 2) - 0.4301*tempBEP + 0.641;
	}
	else {
		Pistd[0] = (q75*h75 / 3956) / (alpha[0] * (effPumpSTD / 100));
		Pistd[1] = (qBEP*hBEP / 3956) / (alpha[1] * (effPumpSTD / 100));
		Pistd[2] = (q110*h110 / 3956) / (alpha[2] * (effPumpSTD / 100));
		
		p[0] = pip75;
		p[1] = pipBEP;
		p[2] = pip110;
		
		temp75 = pip75 / result.motorHP;
		tempBEP = pipBEP / result.motorHP;
		temp110 = pip110 / result.motorHP;

		y[0] = -0.4508*pow(temp75, 3) + 1.2399*pow(temp75, 2) - 0.4301*temp75 + 0.641;
		y[1] = -0.4508*pow(tempBEP, 3) + 1.2399*pow(tempBEP, 2) - 0.4301*tempBEP + 0.641;
		y[2] = -0.4508*pow(temp110, 3) + 1.2399*pow(temp110, 2) - 0.4301*temp110 + 0.641;
	}
	
	Lfull = (result.motorHP / (result.motorEff / 100)) - result.motorHP;
	
	PERcl = 0;
	PERstd = 0;

	for (int j = 0; j < 3; j++) {


		Li = Lfull * y[j];
		PERclLoop = (Li + p[j])/3;
		PERcl = PERcl + PERclLoop;
	}

	//PERstd = getPERstd(Pistd, Lfull, result.motorHP);
	
	PEIcl = PERcl / PERstd;
	return PEIcl;
}

// Calculates PEIcl using the spline data
double getPEIcl_spline(	vector<double> effVals, vector<double> flowVals, vector<double> headVals, double qBEP, double eBEP,	bool runOut, int speed, double C) {
	initData(effVals, flowVals, headVals, qBEP, eBEP, runOut, speed, C);

	PERcl = 0;
	PERstd = 0;

	PERstd = getPERstd(Pistd, Lfull, result.motorHP);
	for (int j = 0; j < 3; j++) {
		Li = Lfull * y[j];
		dip[j] = Li + p[j]; // store the driver input power to put it into the HI template
		PERclLoop = (Li + p[j]) / 3;
		PERcl = PERcl + PERclLoop;
	}

	PEIcl_spline = PERcl / PERstd;
	return PEIcl_spline;
}


double getPEIvl(double* eCoeff, double* hCoeff, double qBEP, double eBEP, bool runOut, int speed, double C) {
	INIReader reader("config.ini");
	double a, b, d;
	//initData(hCoeff, eCoeff, qBEP, eBEP, speed, runOut);

	if (runOut) {
		double pArray[] = { pin60, pin70, pin80, pin90, pinBEP };
		double qArray[] = { q60, q70, q80, q90, qBEP };
		int result = polyfit(qArray, pArray, 5, 1, tempCoeff);
	}
	else {
		double pArray[] = { pin60, pin75, pin90, pinBEP, pin110, pin120 };
		double qArray[] = { q60, q75, q90, qBEP, q110, q120 };
		int result = polyfit(qArray, pArray, 6, 1, tempCoeff);
	}
	pip60 = tempCoeff[1] * q60 + tempCoeff[0];
	pip65 = tempCoeff[1] * q65 + tempCoeff[0];
	pip75 = tempCoeff[1] * q75 + tempCoeff[0];
	pip90 = tempCoeff[1] * q90 + tempCoeff[0];
	pipBEP = tempCoeff[1] * qBEP + tempCoeff[0];
	pip110 = tempCoeff[1] * q110 + tempCoeff[0];
	pip120 = tempCoeff[1] * q120 + tempCoeff[0];

	pip25vl = (0.8 * (pow(q25, 3)/pow(qBEP,3)) + 0.2 * (q25 / qBEP)) * pipBEP;
	pip50vl = (0.8 * (pow(q50, 3) / pow(qBEP, 3)) + 0.2 * (q50 / qBEP)) * pipBEP;
	pip75vl = (0.8 * (pow(q75, 3) / pow(qBEP, 3)) + 0.2 * (q75 / qBEP)) * pipBEP;

	motor result = getMotorHP(pip120);

	Ns = (nomSpeed * pow(qBEP, 0.5)) / pow((hBEP / stages), .75);
	effPumpSTD = -0.85*pow(log(qBEP), 2) - 0.38*log(Ns)*log(qBEP)
		- 11.48*pow(log(Ns), 2) + 17.80*log(qBEP) + 179.80*log(Ns) - (C + 555.6);

	if (runOut) {
		Pistd[0] = ((q65*h65) / 3956) / (alpha[0] * (effPumpSTD / 100));
		Pistd[1] = ((q90*h90) / 3956) / (alpha[1] * (effPumpSTD / 100));
		Pistd[2] = ((qBEP*hBEP) / 3956) / (alpha[2] * (effPumpSTD / 100));

		temp75 = pip65 / result.motorHP;
		tempBEP = pip90 / result.motorHP;
		temp110 = pipBEP / result.motorHP;
	} else {
		Pistd[0] = (q75*h75 / 3956) / (alpha[0] * (effPumpSTD / 100));
		Pistd[1] = (qBEP*hBEP / 3956) / (alpha[1] * (effPumpSTD / 100));
		Pistd[2] = (q110*h110 / 3956) / (alpha[2] * (effPumpSTD / 100));

		temp75 = pip75 / result.motorHP;
		tempBEP = pipBEP / result.motorHP;
		temp110 = pip110 / result.motorHP;
	}

	p_vl[0] = pip25vl;
	p_vl[1] = pip50vl;
	p_vl[2] = pip75vl;
	p_vl[3] = pipBEP;

	Lfull = (result.motorHP / (result.motorEff / 100)) - result.motorHP;
	PERstd = 0;
	PERvl = 0;

	for (int j = 0; j < 3; j++) {
		double temp = Pistd[j] / result.motorHP;
		yi = -0.4508*pow(temp, 3) + 1.2399*pow(temp, 2) - 0.4301*temp + 0.641;
		Li = Lfull * yi;
		PERstdLoop = (Pistd[j] + Li) / 3;
		PERstd = PERstd + PERstdLoop;
	}

	if (result.motorHP <= 5) {
		a = reader.GetReal("Z VALUES", "a5", -1);
		b = reader.GetReal("Z VALUES", "b5", -1);
		d = reader.GetReal("Z VALUES", "c5", -1);
	} else if (result.motorHP > 5 && result.motorHP <= 20) {
		a = reader.GetReal("Z VALUES", "a20", -1);
		b = reader.GetReal("Z VALUES", "b20", -1);
		d = reader.GetReal("Z VALUES", "c20", -1);
	} else if (result.motorHP > 20 && result.motorHP <= 50) {
		a = reader.GetReal("Z VALUES", "a50", -1);
		b = reader.GetReal("Z VALUES", "b50", -1);
		d = reader.GetReal("Z VALUES", "c50", -1);
	} else {
		a = reader.GetReal("Z VALUES", "aMax", -1);
		b = reader.GetReal("Z VALUES", "bMax", -1);
		d = reader.GetReal("Z VALUES", "cMax", -1);
	}

	for (int j = 0; j < 4; j++) {
		double temp = (p_vl[j] / result.motorHP);
		double z = a * pow(temp,2) + b * (p_vl[j] / result.motorHP) + d;
		Li = Lfull * z;
		PERvlLoop = (Li + p_vl[j])/4;
		PERvl = PERvl + PERvlLoop;
	}
	
	PEIvl = PERvl / PERstd;
	return PEIvl;
}

double getPEIvl_spline(vector<double> effVals, vector<double> flowVals, vector<double> headVals, double qBEP, double eBEP, bool runOut, int speed, double C) {
	INIReader reader("config.ini");
	double a, b, d;
	initData(effVals, flowVals, headVals, qBEP, eBEP, runOut, speed, C);

	pip25vl = (0.8 * (pow(q25, 3) / pow(qBEP, 3)) + 0.2 * (q25 / qBEP)) * pipBEP;
	pip50vl = (0.8 * (pow(q50, 3) / pow(qBEP, 3)) + 0.2 * (q50 / qBEP)) * pipBEP;
	pip75vl = (0.8 * (pow(q75, 3) / pow(qBEP, 3)) + 0.2 * (q75 / qBEP)) * pipBEP;

	p_vl[0] = pip25vl;
	p_vl[1] = pip50vl;
	p_vl[2] = pip75vl;
	p_vl[3] = pipBEP;

	if (result.motorHP <= 5) {
		a = reader.GetReal("Z VALUES", "a5", -1);
		b = reader.GetReal("Z VALUES", "b5", -1);
		d = reader.GetReal("Z VALUES", "c5", -1);
	} else if (result.motorHP > 5 && result.motorHP <= 20) {
		a = reader.GetReal("Z VALUES", "a20", -1);
		b = reader.GetReal("Z VALUES", "b20", -1);
		d = reader.GetReal("Z VALUES", "c20", -1);
	} else if (result.motorHP > 20 && result.motorHP <= 50) {
		a = reader.GetReal("Z VALUES", "a50", -1);
		b = reader.GetReal("Z VALUES", "b50", -1);
		d = reader.GetReal("Z VALUES", "c50", -1);
	} else {
		a = reader.GetReal("Z VALUES", "aMax", -1);
		b = reader.GetReal("Z VALUES", "bMax", -1);
		d = reader.GetReal("Z VALUES", "cMax", -1);
	}

	PERstd = 0;
	PERvl = 0;

	PERstd = getPERstd(Pistd, Lfull, result.motorHP);
	for (int j = 0; j < 4; j++) {
		double temp = (p_vl[j] / result.motorHP);
		double z = a * pow(temp, 2) + b * (p_vl[j] / result.motorHP) + d;
		Li = Lfull * z;
		cip[j] = Li + p_vl[j];
		PERvlLoop = (Li + p_vl[j]) / 4;
		PERvl = PERvl + PERvlLoop;
	}

	PEIvl_spline = PERvl / PERstd;
	return PEIvl_spline;
}


double calcTrend(double* coeff, double point) {
	double value;
	value = coeff[4] * pow(point, 4) + coeff[3] * pow(point, 3) + coeff[2] * pow(point, 2) + coeff[1] * point + coeff[0];
	return value;
}

double getPERstd(double* powerInput, double Lfull, double motorPower) {
	int j;
	double yi, Li, PERstdLoop;
	for (j = 0; j < 3; j++) {
		double test = powerInput[j] / motorPower;
		yi = -0.4508*pow(test, 3) + 1.2399*pow(test, 2) - 0.4301*test + 0.641;
		Li = Lfull * yi;
		PERstdLoop = (powerInput[j] + Li) / 3;
		PERstd = PERstd + PERstdLoop;
	}
	//cout << PERstd << endl;
	return PERstd;
}

void initData(vector<double> effVals, vector<double> flowVals, vector<double> headVals, double qBEP, double eBEP, bool runOut, int speed, double C) {
	//Initialize flow, head, and efficiency at all necessary points
	tk::spline effSpline;
	tk::spline headSpline;
	effSpline.set_points(flowVals, effVals);    // X needs to be sorted, strictly increasing
	headSpline.set_points(flowVals, headVals);

	hBEP = headSpline(qBEP);
	q25 = qBEP * 0.25;			h25 = headSpline(q25);			e25 = effSpline(q25);
	q40 = qBEP * 0.40;			h40 = headSpline(q40);			e40 = effSpline(q40);
	q50 = qBEP * 0.50;			h50 = headSpline(q50);			e50 = effSpline(q50);
	q60 = qBEP * 0.60;			h60 = headSpline(q60);			e60 = effSpline(q60);
	q65 = qBEP * 0.65;			h65 = headSpline(q65);			e65 = effSpline(q65);
	q70 = qBEP * 0.70;			h70 = headSpline(q70);			e70 = effSpline(q70);
	q75 = qBEP * 0.75;			h75 = headSpline(q75);			e75 = effSpline(q75);
	q80 = qBEP * 0.80;			h80 = headSpline(q80);			e80 = effSpline(q80);
	q90 = qBEP * 0.90;			h90 = headSpline(q90);			e90 = effSpline(q90);
	q110 = qBEP * 1.10;			h110 = headSpline(q110);		e110 = effSpline(q110);
	q120 = qBEP * 1.20;			h120 = headSpline(q120);		e120 = effSpline(q120);

	if (speed > 1440 && speed < 2160) {
		nomSpeed = 1800;
		//C = 128.85; //CREATE TABLE FOR C VALUES
		poleIndex = 1;
	}
	else {
		nomSpeed = 3600;
		//C = 130.99;
		poleIndex = 2;
	}

	/*//////////////////////////////////////////////////////////////////////////
	Calculate input power =>	hydraulic output power / (efficiency / 100)
	hydraulic output power => head * flow / 3956
	*//////////////////////////////////////////////////////////////////////////
	pin60 = (q60 * h60 / 3956) / (e60 / 100);
	pin70 = (q70 * h70 / 3956) / (e70 / 100);
	pin75 = (q75 * h75 / 3956) / (e75 / 100);
	pin80 = (q80 * h80 / 3956) / (e80 / 100);
	pin90 = (q90 * h90 / 3956) / (e90 / 100);
	pinBEP = (qBEP * hBEP / 3956) / (eBEP / 100);
	pin110 = (q110 * h110 / 3956) / (e110 / 100);
	pin120 = (q120 * h120 / 3956) / (e120 / 100);


	/*===========================================================================
	Use load points of 60%, 75%, 90%, 100%, 110%, and 120% of BEP flow
	and input power. Preform a linear regression to determine the
	input power at the required load points.

	If the pump can not be tested to 120%, use the loading points of
	60%, 70%, 80%, 90%, 100% of BEP flow
	============================================================================*/

	if (runOut) {
		double pArray[] = { pin60, pin70, pin80, pin90, pinBEP };
		double qArray[] = { q60, q70, q80, q90, qBEP };
		int result = polyfit(qArray, pArray, 5, 1, tempCoeff);
	}
	else {
		double pArray[] = { pin60, pin75, pin90, pinBEP, pin110, pin120 };
		double qArray[] = { q60, q75, q90, qBEP, q110, q120 };
		int result = polyfit(qArray, pArray, 6, 1, tempCoeff);
	}

	//tempCoeff[1] and [0] contain the coefficients from the linear regression
	pip60 = tempCoeff[1] * q60 + tempCoeff[0];
	pip65 = tempCoeff[1] * q65 + tempCoeff[0];
	pip75 = tempCoeff[1] * q75 + tempCoeff[0];
	pip90 = tempCoeff[1] * q90 + tempCoeff[0];
	pipBEP = tempCoeff[1] * qBEP + tempCoeff[0];
	pip110 = tempCoeff[1] * q110 + tempCoeff[0];
	pip120 = tempCoeff[1] * q120 + tempCoeff[0];

	result = getMotorHP(pip120);
	Ns = (nomSpeed * pow(qBEP, 0.5)) / pow((hBEP / stages), .75);
	effPumpSTD = -0.85*pow(log(qBEP), 2) - 0.38*log(Ns)*log(qBEP)
		- 11.48*pow(log(Ns), 2) + 17.80*log(qBEP) + 179.80*log(Ns) - (C + 555.6);


	//Put all the values needed for the PEI calculation into their seats
	if (runOut) {
		Pistd[0] = ((q65*h65) / 3956) / (alpha[0] * (effPumpSTD / 100));
		Pistd[1] = ((q90*h90) / 3956) / (alpha[1] * (effPumpSTD / 100));
		Pistd[2] = ((qBEP*hBEP) / 3956) / (alpha[2] * (effPumpSTD / 100));

		p[0] = pip65;
		p[1] = pip90;
		p[2] = pipBEP;

		temp65 = pip65 / result.motorHP;
		temp90 = pip90 / result.motorHP;
		tempBEP = pipBEP / result.motorHP;

		y[0] = -0.4508*pow(temp65, 3) + 1.2399*pow(temp65, 2) - 0.4301*temp65 + 0.641;
		y[1] = -0.4508*pow(temp90, 3) + 1.2399*pow(temp90, 2) - 0.4301*temp90 + 0.641;
		y[2] = -0.4508*pow(tempBEP, 3) + 1.2399*pow(tempBEP, 2) - 0.4301*tempBEP + 0.641;
	}
	else {
		Pistd[0] = (q75*h75 / 3956) / (alpha[0] * (effPumpSTD / 100));
		Pistd[1] = (qBEP*hBEP / 3956) / (alpha[1] * (effPumpSTD / 100));
		Pistd[2] = (q110*h110 / 3956) / (alpha[2] * (effPumpSTD / 100));

		p[0] = pip75;
		p[1] = pipBEP;
		p[2] = pip110;

		temp75 = pip75 / result.motorHP;
		tempBEP = pipBEP / result.motorHP;
		temp110 = pip110 / result.motorHP;

		y[0] = -0.4508*pow(temp75, 3) + 1.2399*pow(temp75, 2) - 0.4301*temp75 + 0.641;
		y[1] = -0.4508*pow(tempBEP, 3) + 1.2399*pow(tempBEP, 2) - 0.4301*tempBEP + 0.641;
		y[2] = -0.4508*pow(temp110, 3) + 1.2399*pow(temp110, 2) - 0.4301*temp110 + 0.641;
	}

	Lfull = (result.motorHP / (result.motorEff / 100)) - result.motorHP;

	//hBEP = calcTrend(hCoeff, qBEP);

 // /*qXX => flow @ XX% BEP flow		hXX => head	@ XX% BEP flow					eXX => efficiency @	XX% BEP flow*/	
	//q25  = qBEP * 0.25;				h25  = calcTrend(hCoeff, q25);				e25  = calcTrend(eCoeff, q25);
	//q40  = qBEP * 0.40;				h40  = calcTrend(hCoeff, q40);				e40  = calcTrend(eCoeff, q40);
	//q50  = qBEP * 0.50;				h50  = calcTrend(hCoeff, q50);				e50  = calcTrend(eCoeff, q50);
	//q60  = qBEP * 0.60;				h60  = calcTrend(hCoeff, q60);				e60  = calcTrend(eCoeff, q60);
	//q65  = qBEP * 0.65;				h65  = calcTrend(hCoeff, q65);				e65  = calcTrend(eCoeff, q65);
	//q70  = qBEP * 0.70;				h70  = calcTrend(hCoeff, q70);				e70  = calcTrend(eCoeff, q70);
	//q75  = qBEP * 0.75;				h75  = calcTrend(hCoeff, q75);				e75  = calcTrend(eCoeff, q75);
	//q80  = qBEP * 0.80;				h80  = calcTrend(hCoeff, q80);				e80  = calcTrend(eCoeff, q80);
	//q90  = qBEP * 0.90;				h90  = calcTrend(hCoeff, q90);				e90  = calcTrend(eCoeff, q90);
	//q110 = qBEP * 1.10;				h110 = calcTrend(hCoeff, q110);				e110 = calcTrend(eCoeff, q110);
	//q120 = qBEP * 1.20;				h120 = calcTrend(hCoeff, q120);				e120 = calcTrend(eCoeff, q120);

	//q25spline = qBEP * 0.25;				h25spline = calcTrend(hCoeff, q25spline);				e25spline = calcTrend(eCoeff, q25spline);
	//q40spline = qBEP * 0.40;				h40spline = calcTrend(hCoeff, q40spline);				e40spline = calcTrend(eCoeff, q40spline);
	//q50spline = qBEP * 0.50;				h50spline = calcTrend(hCoeff, q50spline);				e50spline = calcTrend(eCoeff, q50spline);
	//q60spline = qBEP * 0.60;				h60spline = calcTrend(hCoeff, q60spline);				e60spline = calcTrend(eCoeff, q60spline);
	//q65spline = qBEP * 0.65;				h65spline = calcTrend(hCoeff, q65spline);				e65spline = calcTrend(eCoeff, q65spline);
	//q70spline = qBEP * 0.70;				h70spline = calcTrend(hCoeff, q70spline);				e70spline = calcTrend(eCoeff, q70spline);
	//q75spline = qBEP * 0.75;				h75spline = calcTrend(hCoeff, q75spline);				e75spline = calcTrend(eCoeff, q75spline);
	//q80spline = qBEP * 0.80;				h80spline = calcTrend(hCoeff, q80spline);				e80spline = calcTrend(eCoeff, q80spline);
	//q90spline = qBEP * 0.90;				h90spline = calcTrend(hCoeff, q90spline);				e90spline = calcTrend(eCoeff, q90spline);
	//q110spline = qBEP * 1.10;				h110spline = calcTrend(hCoeff, q110spline);				e110spline = calcTrend(eCoeff, q110spline);
	//q120spline = qBEP * 1.20;				h120spline = calcTrend(hCoeff, q120spline);				e120spline = calcTrend(eCoeff, q120spline);
	////Determine nominal speed and C value
	//if (speed > 1440 && speed < 2160) {
	//	nomSpeed = 1800;
	//	//C = 128.85; //CREATE TABLE FOR C VALUES
	//	poleIndex = 1;
	//}
	//else {
	//	nomSpeed = 3600;
	//	//C = 130.99;
	//	poleIndex = 2;
	//}

	///*//////////////////////////////////////////////////////////////////////////
	//	Calculate input power =>	hydraulic output power / (efficiency / 100)
	//	hydraulic output power => head * flow / 3956						
	//*//////////////////////////////////////////////////////////////////////////
	///*pin60spline = (q60spline * h60spline / 3956) / (e60spline / 100);
	//pin70spline = (q70spline * h70spline / 3956) / (e70spline / 100);
	//pin75spline = (q75spline * h75spline / 3956) / (e75spline / 100);
	//pin80spline = (q80spline * h80spline / 3956) / (e80spline / 100);
	//pin90spline = (q90spline * h90spline / 3956) / (e90spline / 100);
	//pinBEPspline = (qBEPspline * hBEPspline / 3956) / (eBEPspline / 100);
	//pin110spline = (q110spline * h110spline / 3956) / (e110spline / 100);
	//pin120spline = (q120spline * h120spline / 3956) / (e120spline / 100);*/

	//pin60 = (q60 * h60 / 3956) / (e60 / 100);
	//pin70 = (q70 * h70 / 3956) / (e70 / 100);
	//pin75 = (q75 * h75 / 3956) / (e75 / 100);
	//pin80 = (q80 * h80 / 3956) / (e80 / 100);
	//pin90 = (q90 * h90 / 3956) / (e90 / 100);
	//pinBEP = (qBEP * hBEP / 3956) / (eBEP / 100);
	//pin110 = (q110 * h110 / 3956) / (e110 / 100);
	//pin120 = (q120 * h120 / 3956) / (e120 / 100);

	
}



motor getMotorHP(double pow) {
	bool horse = true;
	int i = 0;
	//double motorHP;
	motor result;
	
	while (horse) {
		//cout << pow << endl;
		if (pow > hpArray.at(i)) {
			i++;
			if (pow < hpArray.at(i)) 
			{
				result.motorHP = hpArray.at(i);
				if (poleIndex == 1) {
					result.motorEff = effArray4.at(i);
				}
				else {
					result.motorEff = effArray2.at(i);
				}
				horse = false;
			}
			
		} else {
			result.motorHP = hpArray.at(i);
			if (poleIndex == 1) {
				result.motorEff = effArray4.at(i);
			}
			else {
				result.motorEff = effArray2.at(i);
			}
			horse = false;
		}


		if (pip120 > 200) {
			horse = false;
			result.motorHP = 200;
			result.motorEff = 100;
		}
	}
	//cout << result.motorHP << endl;
	return result;
}