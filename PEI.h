#ifndef PEI_H_
#define PEI_H_
#include "spline.h"
#include <vector>

	extern double PERcl, PERstd, PERvl;
	extern double PEIcl;

	extern double q25, q40, q50, q60, q65, q70, q75, q80, q90, q110, q120;
	extern double q25spline, q40spline, q50spline, q60spline, q65spline, q70spline, q75spline, q80spline, q90spline, q110spline, q120spline;
	extern double h25, h40, h50, h60, h65, h70, h75, h80, h90, hBEP, h110, h120;
	extern double pip25, pip60, pip65, pip75, pip90, pipBEP, pip110, pip120;
	extern double Pistd[3], p[3], p_vl[4], y[3], tempCoeff[2];
	//using namespace std;

	struct motor {
		double motorHP;
		double motorEff;
	};

	//motor result;
	double getPEIcl(double* eCoeff, double* hCoeff, double qBEP, double eBEP, bool runOut, int speed, double C);
	double getPEIcl_spline(std::vector<double> effVals, std::vector<double> flowVals, std::vector<double> headVals, double qBEP, double eBEP, bool runOut, int speed, double C);
	double getPEIvl(double* eCoeff, double* hCoeff, double qBEP, double eBEP, bool runOut, int speed, double C);
	double getPEIvl_spline(std::vector<double> effVals, std::vector<double> flowVals, std::vector<double> headVals, double qBEP, double eBEP, bool runOut, int speed, double C);
	double getPERstd(double* powerInput, double Lfull, double motorPower);
	void initData(std::vector<double> effVals, std::vector<double> flowVals, std::vector<double> headVals, double qBEP, double eBEP, bool runOut, int speed, double C);
	double calcTrend(double* coeff, double point);
	motor getMotorHP(double pow);

	
	//string pumpType;
	
	



#endif


	/*Motor HP	4 Pole		2 Pole*/
	//extern double motorArray[20][3] =      {{1.000,		85.5,		77.0 },
	//										{1.500,		86.5,		84.0 },			
	//										{2.000,		86.5,		85.5 },			
	//										{3.000,		89.5,		85.5 },
	//										{5.000,		89.5,		86.5 },
	//										{7.500,		91.0,		88.5 },
	//										{10.00,		91.7,		89.5 },
	//										{15.00,		92.4,		90.2 },
	//										{20.00,		93.0,		91.0 },
	//										{25.00,		93.6,		91.7 },
	//										{30.00,		93.6,		91.7 },
	//										{40.00,		94.1,		92.4 },
	//										{50.00,		94.5,		93.0 },
	//										{60.00,		95.0,		93.6 },
	//										{75.00,		95.0,		93.6 },
	//										{100.0,		95.4,		93.6 },
	//										{125.0,		95.4,		94.1 },
	//										{150.0,		95.8,		94.1 },
	//										{200.0,		95.8,		95.0 },
	//										{250.0,		95.8,		95.0 } };