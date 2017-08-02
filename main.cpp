/*############################################################################################################################
*This program is deisgned to calculate PEI value. It was created in visual studios with c++. 
*
*	Author: Alex Snarski
*
*	I created this program while interning for Xylem. If there is a problem
*	with the program and I am not around, feel free to contact me at: alexandersnarski2018@u.northwestern.edu
*
*	The program needs the following files in order to run:
*			1. main.cpp (this file)
*				- Takes the input .csv file and stores the data contained there
*				- Sifts through the raw data to seperate out the different pumps
*
*			2. PEI.cpp			-> PEI.h
*				-Sole purpose is to calculate constant and variable load PEI values
*		
*			3. database.cpp		-> database.h
*				- Takes the calculated data and stores it in an excel spreadsheet
*				- Creates 4 seperate worksheets:
*					2 that show the minimal information regarding the PEI values
*					2 that show the data in the same format as the HI database template
*
*			4. polyfit.cpp		-> polyfit.h
*				- Takes the sorted data points for a given pump, and creates a polynomial curve fit
*
*			5. INIreader.cpp	-> INIreader.h
*				- Also includes ini.cpp and ini.h (relient on each other)
*				- Reads the ini file for certain variables
*
*			6. spline.h
*				- Whole file contained in the header
*				- Creates a cubic spline interpolation for the data set
*
*			7. XlsxWriter.cpp	-> xlsxwriter.h
*				- Contains the classes and functions that allow writing to a .xlsx file
*				- Currently there is no support to read an excel file
*					Note:	If a library to read .xlsx files becomes available, it might be easier
*							to implement that library, rather than needing to convert to a .csv file
*							each time
*
*	The external functions are listed below, along with a link to their documentation.
*		libxlsxwriter	-	https://libxlsxwriter.github.io/index.html
*		spline			-	https://github.com/ttk592/spline
*		polyfit			-	https://github.com/natedomin/polyfit
*		INIreader		-	https://github.com/benhoyt/inih
*
*	A couple helpful pages for some common C++ syntax:
*		General Info - http://www.cplusplus.com/doc/tutorial/
*		String Class - http://www.cplusplus.com/reference/string/string/
*		Vector Class - http://www.cplusplus.com/reference/vector/vector/
*		I/O Streams  - http://www.cplusplus.com/reference/fstream/ifstream/
*
##############################################################################################################################*/

#include "stdafx.h"
#include <iostream>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <list>
#include <cmath>
#include <fstream>
#include <iomanip>
#include "polyfit.h"
#include "PEI.h"
#include "IniReader.h"
#include "spline.h"
#include "xlsxwriter.h"
#include <ctime>
#include "database.h"

#define ORDER 4
#define config "config.ini" // If the name of the config file changes, this #define must also be changed


using namespace std;															
																				// [VARIABLE DEFINITIONS]
// Function prototypes															
void storeData(string file);													// Takes data from the .csv file and stores it into arrays
double readCvalue(string configFile, string pType);								// Reads the C value from the config file for a given pumps parameters
void readConfig(string file);													// Reads the rest of the config file

// Vectors for storing the raw data from the .csv file
vector<string> seriesArray;														// Stores the series (ex: e-1510), usually a constant for a given document
vector<string> modelArray;														// Stores all the model names (ex: 1.25AD or 5A)
vector<int> speedArray;															// Stores the testing speed for each data point (rpm)
vector<double> impArray;														// Stores the impeller diameter for each model (inches)
vector<double> flowArray;														// Stores the flow data points (GPM)
vector<double> headArray;														// Stores the head data points (feet)
vector<double> effArray;														// Stores the efficiency data points (%)

//These vectors store the motor data taken from the config file
vector<double> hpArray;															// Stores the nominal horsepower
vector<double> effArray2;														// Stores the 2 pole efficiencies
vector<double> effArray4;														// Stores the 4 pole efficiencies

string file = "test.csv";														// Name of the .csv document containing the data

// These are stored as global variables so that the database functions can access them
string pumpType, brand, manufacturer;											// pumpType:		the type of pump being tested, read from config (ex: ESFM, ESCC, IL)
extern lxw_workbook *workbook;													// brand:			the brand of the pump, read from config (ex: Bell & Gossett, Gould's)
double flow, eff, flowMax, effMax, flowBEP, effSplineMax, flowSplineBEP;		// manufacturer:	the company that sells the pump, probably always will be Xylem, read from config
bool runOut;																	// workbook:		the workbook class used by libxlsxwriter
int labNumber, stages;															// runOut:			a boolean that says if the pump can be tested to 120% BEP flow
																				// labNumber:		the HI approved lab number, read from config
																				// stages:			the number of stages of the pump, read from config
																				
/*############################################################################

							MAIN FUNCTION

#############################################################################*/
int main() {																	// len:			total number of rows in the raw data document
	int len, j, row, poly, N, ii, speed, nomSpeed;								// j:			index used as a placeholder for the begining of each models data
	double delta, headCoeff[5], effCoeff[5];									// row:			row number for the database to write in (1 row for each model)
																				// poly:		takes the return value from the polyfit function, which is always 0
	ifstream myFile(file);														// N:			total number of datapoints for the current model
	storeData(file);															// ii:			general index used to increment through the elements of an array
	readConfig(config);															// speed:		the tested speed for a given data point
																				// nomSpeed:	nominal speed for the pump, either 1800 or 3600 rpm
	string strTemp = seriesArray.at(1) + " PEI Data" + ".xlsx";					// delta:		the step increase used when finding the max efficiency
	const char * fileName = strTemp.c_str();									// headCoeff:	stores the coefficients for the head vs flow polynomial
	dataInit(fileName);															// effCoeff:	stores the coefficients for the efficiency vs flow polynomial
																				// fileName:	the name of the excel document that gets created, named after the series
	len = seriesArray.size();													
	j = 0;																		
	row = 1;																	
																				
	for (int i = 0; i <= len; i++) {											
		if (i == len)															
			break;																
																				
		 /*len is the amount of rows in the .csv file							
		 i is the index to each row, with the first								
		 row being referenced as row 0. Because of this,						
		 i - 1 will be the last row in the data arrays, and						
		 there will be no data at the the location where i = len.				
		 To counter this, I created a statement for when the loop				
		 gets to the last line of the file. When i == len - 1,					
		 the program will set the next speed and model name to					
		 and arbitrary value, allowing the program to enter the main			
		 if statement, and store the last set of data*/							
																				
		tk::spline effSpline;													// effSpline:	a class used to store the efficiency data for the spline calculation
		tk::spline headSpline;													// headSpline:	a class used to store the head data for the spline calculation 
																				
		string modelNow = modelArray.at(i);										// modelNow:	the model stored in element i
		string modelNext;														// modelNext:	the model stored in element i + 1
		int speedNow = speedArray.at(i);										// speedNow:	the speed stored in element i
		int speedNext;															// speedNext:	the speed stored in element i + 1
																							
																				
		if (i == len - 1) {														
			speedNext = 0;														
			modelNext = 'a';													
		}																		
		else {																	
			modelNext = modelArray.at(i + 1);									
			speedNext = speedArray.at(i + 1);									
		}																		
																				
																				
			/*================================================					
			Checks the model and speed in the current position					
			and compares it to the next position.								
																				
			If either of the models or speeds are different,					
			the program will enter the if statement below						
			==================================================*/				
		if ((modelNow != modelNext) || (speedNow != speedNext)) {				
			flowMax = 0;	flowBEP = 0;	flow = 0;							// flowMax:		the max flow of the current dataset
			effMax = 0;		eff = 0;	ii = 0;		effSplineMax = 0;			// flowBEP:		the BEP flow point for the current dataset
																				// flow:		the flow used in finding the BEP point
			N = i - j + 1;														// N:			the amount of data points in the current dataset
			double* flowVals = new double[N];									// flowVals:	used to store the flow datapoints for the current dataset
			double* effVals = new double[N];									// effVals:		used to store the efficiency datapoints for the current dataset
			double* headVals = new double[N];									// headVals:	used to store the head datapoints for the current dataset
			vector<double> flowVals_spline, effVals_spline, headVals_spline;	//		Note: the spline variants of the above serve the same purpose
			speed = speedArray.at(i);											
																																								
			//Get the nominal speed, ranges based on the DOE guidelines			
			if (speed > 1440 && speed < 2160) {									
				nomSpeed = 1800;		// 4 Pole								
			}																	
			else {																
				nomSpeed = 3600;		// 2 Pole								
			}																	
																				
			//Create a string in the form of pumpType.nomSpeed (ex: ESFM.1800)	
			string speedString = to_string(nomSpeed);							
			string pType = pumpType + "." + speedString;						
			double C = readCvalue(config, pType);								// C:			the C value for the pumps configuration, read from the ini file
																				
																				
			/*===========================================================		
			This loop stores the data into an array so it can					
			used to create a curve fit. Here, the variable j is					
			the placeholder for the begining of the data set, and i				
			is the placeholder for the last set of values						
																				
			The program will take the head, flow, and efficiency values			
			at the jth location in the array each loop until j reached i		
			=============================================================*/		
			while (j <= i) {													
				double affinity = (nomSpeed / speedArray.at(j));				// affinity:	used to convert the data to nominal speeds
				flowVals[ii] = flowArray.at(j)*affinity;						
				effVals[ii] = effArray.at(j);									
				headVals[ii] = headArray.at(j)*pow(affinity, 2);				
				flowVals_spline.push_back(flowArray.at(j)*affinity);			
				effVals_spline.push_back(effArray.at(j));						
				headVals_spline.push_back(headArray.at(j)*pow(affinity, 2));	
				if (flowArray.at(j) > flowMax)									
					flowMax = flowArray.at(j);									
				ii++;															
				j++;															
			}																	
																				
			//For polynomial curve fitting, not needed if spline is being used	
			poly = polyfit(flowVals, effVals, N, ORDER, effCoeff);				
			poly = polyfit(flowVals, headVals, N, ORDER, headCoeff);			
																				
			effSpline.set_points(flowVals_spline, effVals_spline);				
			headSpline.set_points(flowVals_spline, headVals_spline);			
																				
			// Take the max flow and divide it by 1000							
			delta = flowMax / 1000.00;											
			while (flow <= flowMax) {											
				/*eff = effCoeff[4] * pow(flow, 4) + effCoeff[3] * pow(flow, 3)	
					+ effCoeff[2] * pow(flow, 2) + effCoeff[1]					
					* flow + effCoeff[0];*/		

				double effS = effSpline(flow);		
				// Compare the new calculated efficieny vs the known max		
				if (effS > effSplineMax) {										
					effSplineMax = effS;										
					flowSplineBEP = flow;										
				}																
				if (eff > effMax) {												
					effMax = eff;												
					flowBEP = flow;												
				}																
																				
				// starts at flow = 0, adds delta until it reaches flowMax		
				flow = flow + delta;											
			}																	
																				
			//Check to see if the pump can be tested to 120% BEP flow			
			if (flowBEP*1.2 > flowMax)											// runOut:		a boolean that says if the pump can be tested to 120% BEP flow
				runOut = true;													//		Note: If runOut is true, it can't be tested to 120% BEP flow
			else																
				runOut = false;
			

			//These functions calculated the PEI values based on the polynomial curve fit, and are obsolete
			//double PEIvl = getPEIvl(effCoeff, headCoeff, flowBEP, effMax, runOut, speed, C);
			//double PEIcl = getPEIcl(effCoeff, headCoeff, flowBEP, effMax, runOut, speed, C);

			//Calculate the PEI values for constant load and variable load, and then send the data to be stored in a spreadsheet
			double PEIcl_spline = getPEIcl_spline(effVals_spline, flowVals_spline, headVals_spline, flowSplineBEP, effSplineMax, runOut, speed, C);
			writeData_cl(i, row);

			double PEIvl_spline = getPEIvl_spline(effVals_spline, flowVals_spline, headVals_spline, flowSplineBEP, effSplineMax, runOut, speed, C);
			writeData_vl(i, row);

			//Print the data to the console for debugging purposes
			cout << "Model:" << modelArray.at(i) << " " << speed << endl;
			cout << "Spline PEIcl:	" << PEIcl_spline << endl;
			cout << "Spline PEIvl:	" << PEIvl_spline << endl;
			cout << endl;

			//Need to delete/clear the arrays used for storing the data points to be able to resize them next loop
			delete[] flowVals;
			delete[] effVals;
			delete[] headVals;
			flowVals_spline.clear();
			headVals_spline.clear();
			effVals_spline.clear();
			row++;
		}

	}
	// Close the workbook in order to save the data
	workbook_close(workbook);
	return 0;
}



/*=============================================================================
		This function reads the .csv file and seperates it into arrays
		Each array contains one type of data from the .csv file
		The order in the csv file must be the following:
			Col 1: Series
			Col 2: Model
			Col 3: Speed
			Col 4: Impeller Diameter
			Col 5: Flow
			Col 6: Head
			Col 7: Efficiency
==============================================================================*/
void storeData(string file) {													// file:		contains the name of the .csv file
	string str, seriesTemp, modelTemp, token;									// str:			a string that stores an entire row from the .csv file
	int speedTemp;																// seriesTemp:	takes the series data from str
	double flowTemp, headTemp, effTemp, impTemp;								// modelTemp:	takes the model data from str
	bool read = true;															// flowTemp:	takes the flow data from str
	bool index = true;															// headTemp:	takes the head data from str
	ifstream myFile(file);														// effTemp:		takes the efficiency data from str
																				// impTemp:		takes the impeller data from str
	while (read) {																// read:		a boolean that is set false when the end of the file is reached
		if (index) {															// index:		a boolean that is set false after the first line is read
			// The first line is the header, can be discarded					
			getline(myFile, str, '\n'); 
			istringstream buffer(str);											
			index = false;
		}
		getline(myFile, str, '\n'); //The function getline reads the file until it reaches the character you specified, in this case the newline character
		istringstream buffer(str);  //Put the string into the stream buffer

		getline(buffer, token, ','); //Use getline again to find a comma, and then take everything before it 
		seriesTemp = token;
		seriesArray.push_back(seriesTemp); //push_back puts the new value at the very back of the array (http://www.cplusplus.com/reference/vector/vector/push_back/)
											
		getline(buffer, token, ',');
		modelTemp = token;
		modelArray.push_back(modelTemp);

		getline(buffer, token, ',');
		speedTemp = stoi(token);
		speedArray.push_back(speedTemp);

		getline(buffer, token, ',');
		impTemp = 0;
		impArray.push_back(impTemp);

		getline(buffer, token, ',');
		flowTemp = stod(token);
		flowArray.push_back(flowTemp);

		getline(buffer, token, ',');
		headTemp = stod(token);
		headArray.push_back(headTemp);


		getline(buffer, token, ',');
		effTemp = stod(token);
		effArray.push_back(effTemp);
		
		if (myFile.eof()) {
			read = false;
			myFile.close();
		}
	}
}

/*===================================================================================================
		The ini reader for the C values gets it's own function because it is used in every loop
====================================================================================================*/
double readCvalue(string configFile, string pType) {
	INIReader reader(configFile);
	double C = reader.GetReal("C VALUES", pType, -1);
	return C;
}

/*===================================================================================================
		This function will search the ini file for the parameters that are stored there
====================================================================================================*/
void readConfig(string file) {
	
	INIReader reader(file);
																				
	int motorRows = reader.GetInteger("MOTOR", "rows", 21);						// motorRows:	the number of rows for the motor array in the ini file
	
	for (int i = 0; i < motorRows; i++) {
		string buffer = "row" + to_string(i);									// buffer:		tells the function what to look for in the config file
		string str = reader.Get("MOTOR", buffer, "-1");
		ifstream myFile(file);
		istringstream buf(str);
		string token;
		double temp;

		getline(buf, token, ',');
		temp = stod(token); // http://www.cplusplus.com/reference/string/stod/
		hpArray.push_back(temp);

		getline(buf, token, ',');
		temp = stod(token);
		effArray4.push_back(temp);

		getline(buf, token, ',');
		temp = stod(token);
		effArray2.push_back(temp);
	}
	pumpType = reader.Get("PUMP CONFIG", "pumpType", "ESFM");
	stages = reader.GetInteger("PUMP CONFIG", "stages", 1);
	brand = reader.Get("PUMP CONFIG", "brand", "Bell & Gossett");
	manufacturer = reader.Get("PUMP CONFIG", "manufacturer", "Xylem");
	labNumber = reader.GetInteger("PUMP CONFIG", "labNumber", 102);
}

//This function isn't used currently, but can be used to get the date for file naming purposes
//string getTime() {
//	string temp;
//	time_t now = time(0);
//	tm *time = localtime(&now);
//	int year = 1900 + time->tm_year; // Time since 1900
//	int month = 1 + time->tm_mon; // Month from 0-11
//	int day = time->tm_mday; // Day from 1-31
//	int hour = time->tm_hour; // Hour from 0-24
//	int min = 1 + time->tm_min; // Min from 0-59
//	int sec = 1 + time->tm_sec; // Sec from 0-61
//	if (hour < 12)
//		temp = "am";
//	else
//		temp = "pm";
//
//	string date = to_string(month) + "_" + to_string(day) + "_" + to_string(year) + " " + to_string(hour) + "" + to_string(min);
//	return date;
//}