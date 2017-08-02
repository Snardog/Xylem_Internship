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
#include "PEI.h"
#include "xlsxwriter.h"
#include "database.h"


using namespace std;

/*#####################################################################################################
	Most of the variables used in this section are global variables in either main.cpp or PEI.cpp
  This files sole purpose is to take the calculated values and store them into an excel spreadsheet

  Documentation for LibXlsxWriter: https://libxlsxwriter.github.io/index.html
######################################################################################################*/

extern vector<string> seriesArray;
extern vector<string> modelArray;
extern vector<int> speedArray;
extern vector<double> impArray;

extern string pumpType, brand, manufacturer;

lxw_workbook *workbook;
lxw_worksheet *PEIcl_sheet;
lxw_worksheet *PEIvl_sheet;
lxw_worksheet *HI_cl_sheet;
lxw_worksheet *HI_vl_sheet;

void dataInit(const char* fileName) {
	workbook = workbook_new(fileName);

	lxw_format *header_middle = workbook_add_format(workbook);
	format_set_text_wrap(header_middle);
	format_set_align(header_middle, LXW_ALIGN_CENTER_ACROSS);
	format_set_align(header_middle, LXW_ALIGN_VERTICAL_CENTER);
	format_set_top(header_middle, LXW_BORDER_THICK);
	format_set_bottom(header_middle, LXW_BORDER_THICK);
	format_set_fg_color(header_middle, 0xC6B486);

	////////////////////////////////////////////////////////
	PEIcl_sheet = workbook_add_worksheet(workbook, "PEIcl");
	////////////////////////////////////////////////////////

	worksheet_write_string(PEIcl_sheet, 0, 0, "Series", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 1, "Model", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 2, "Impeller Diameter", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 3, "Speed", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 4, "Pump Type", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 5, "Max Efficiency", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 6, "120% BEP Flow Possible?", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 7, "Flow (GPM)", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 8, "Head (ft)", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 9, "Pump Input Power (Hp)", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 11, "PERstd", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 12, "PERcl", header_middle);
	worksheet_write_string(PEIcl_sheet, 0, 13, "PEIcl", header_middle);

	////////////////////////////////////////////////////////
	PEIvl_sheet = workbook_add_worksheet(workbook, "PEIvl");
	////////////////////////////////////////////////////////

	worksheet_write_string(PEIvl_sheet, 0, 0, "Series", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 1, "Model", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 2, "Impeller Diameter", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 3, "Speed", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 4, "Pump Type", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 5, "Max Efficiency", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 6, "120% BEP Flow Possible?", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 7, "Flow (GPM)", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 8, "Head (ft)", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 9, "Pump Input Power (Hp)", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 11, "PERstd", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 12, "PERvl", header_middle);
	worksheet_write_string(PEIvl_sheet, 0, 13, "PEIvl", header_middle);

	HI_cl_sheet = workbook_add_worksheet(workbook, "HI TEMPLATE CL");
	HI_vl_sheet = workbook_add_worksheet(workbook, "HI TEMPLATE VL");
}

void writeData_cl(int data, int row) {
	const char* series = seriesArray.at(data).c_str();
	const char* model = modelArray.at(data).c_str();
	const char* pType = pumpType.c_str();
	const char* brandx = brand.c_str();
	const char* manufacturerX = manufacturer.c_str();
	const char* pumpConfig = "Bare Pump";
	int speed = speedArray.at(data);
	double impDiameter = impArray.at(data);
	extern double effMax, dip[];
	extern bool runOut;
	extern double PEIcl_spline, flowBEP;
	extern int stages, nomSpeed, labNumber;
	extern motor result;

	const char* temp;
	if (runOut)
		temp = "No";
	else
		temp = "Yes";

	worksheet_write_string(PEIcl_sheet, row, 0, series, NULL);
	worksheet_write_string(PEIcl_sheet, row, 1, model, NULL);
	worksheet_write_number(PEIcl_sheet, row, 2, impDiameter, NULL);
	worksheet_write_number(PEIcl_sheet, row, 3, speed, NULL);
	worksheet_write_string(PEIcl_sheet, row, 4, pType, NULL);
	worksheet_write_number(PEIcl_sheet, row, 5, effMax, NULL);
	worksheet_write_string(PEIcl_sheet, row, 6, temp, NULL);
	worksheet_write_number(PEIcl_sheet, row, 7, flowBEP, NULL);
	worksheet_write_number(PEIcl_sheet, row, 8, hBEP, NULL);
	worksheet_write_number(PEIcl_sheet, row, 9, pipBEP, NULL);
	worksheet_write_number(PEIcl_sheet, row, 11, PERstd, NULL);
	worksheet_write_number(PEIcl_sheet, row, 12, PERcl, NULL);
	worksheet_write_number(PEIcl_sheet, row, 13, PEIcl_spline, NULL);


	string str = seriesArray.at(data) + " " + modelArray.at(data) + " " + to_string(speed) + " " + "CL";
	const char* basicModel = str.c_str();

	worksheet_write_string(HI_cl_sheet, row, 0, manufacturerX, NULL);
	worksheet_write_string(HI_cl_sheet, row, 1, brandx, NULL);
	worksheet_write_string(HI_cl_sheet, row, 2, basicModel, NULL);
	worksheet_write_string(HI_cl_sheet, row, 3, basicModel, NULL);
	worksheet_write_string(HI_cl_sheet, row, 4, pType, NULL);
	worksheet_write_string(HI_cl_sheet, row, 5, pumpConfig, NULL);
	worksheet_write_number(HI_cl_sheet, row, 6, impDiameter, NULL);
	worksheet_write_number(HI_cl_sheet, row, 7, 3, NULL);
	worksheet_write_number(HI_cl_sheet, row, 8, stages, NULL);
	worksheet_write_number(HI_cl_sheet, row, 9, nomSpeed, NULL);
	worksheet_write_number(HI_cl_sheet, row, 13, result.motorHP, NULL);
	worksheet_write_string(HI_cl_sheet, row, 14, temp, NULL);
	worksheet_write_number(HI_cl_sheet, row, 15, flowBEP, NULL);
	worksheet_write_number(HI_cl_sheet, row, 20, hBEP, NULL);
	if (runOut) {
		worksheet_write_number(HI_cl_sheet, row, 25, dip[2], NULL); //BEP driver input power
		worksheet_write_number(HI_cl_sheet, row, 28, dip[0], NULL); //65%
		worksheet_write_number(HI_cl_sheet, row, 29, dip[1], NULL); //90%
		worksheet_write_number(HI_cl_sheet, row, 37, h65, NULL);
		worksheet_write_number(HI_cl_sheet, row, 38, h90, NULL);
	}
	else {
		worksheet_write_number(HI_cl_sheet, row, 25, dip[1], NULL); //BEP driver input power
		worksheet_write_number(HI_cl_sheet, row, 26, dip[0], NULL); //75% BEP
		worksheet_write_number(HI_cl_sheet, row, 27, dip[2], NULL); //110% BEP
		worksheet_write_number(HI_cl_sheet, row, 35, h75, NULL);
		worksheet_write_number(HI_cl_sheet, row, 36, h110, NULL);
	}
	worksheet_write_number(HI_cl_sheet, row, 34, PEIcl_spline, NULL);
	worksheet_write_number(HI_cl_sheet, row, 39, labNumber, NULL);
}

void writeData_vl(int data, int row) {
	const char* series = seriesArray.at(data).c_str();
	const char* model = modelArray.at(data).c_str();
	const char* pType = pumpType.c_str();
	const char* brandx = brand.c_str();
	const char* manufacturerX = manufacturer.c_str();
	const char* pumpConfig = "Bare Pump";
	int speed = speedArray.at(data);
	double impDiameter = impArray.at(data);
	extern double effMax, cip[];
	extern bool runOut;
	extern double PEIvl_spline, flowBEP;
	extern int stages, nomSpeed, labNumber;
	extern motor result;

	const char* temp;
	if (runOut)
		temp = "No";
	else
		temp = "Yes";
	worksheet_write_string(PEIvl_sheet, row, 0, series, NULL);
	worksheet_write_string(PEIvl_sheet, row, 1, model, NULL);
	worksheet_write_number(PEIvl_sheet, row, 2, impDiameter, NULL);
	worksheet_write_number(PEIvl_sheet, row, 3, speed, NULL);
	worksheet_write_string(PEIvl_sheet, row, 4, pType, NULL);
	worksheet_write_number(PEIvl_sheet, row, 5, effMax, NULL);
	worksheet_write_string(PEIvl_sheet, row, 6, temp, NULL);
	worksheet_write_number(PEIvl_sheet, row, 7, flowBEP, NULL);
	worksheet_write_number(PEIvl_sheet, row, 8, hBEP, NULL);
	worksheet_write_number(PEIvl_sheet, row, 9, pipBEP, NULL);
	worksheet_write_number(PEIvl_sheet, row, 11, PERstd, NULL);
	worksheet_write_number(PEIvl_sheet, row, 12, PERvl, NULL);
	worksheet_write_number(PEIvl_sheet, row, 13, PEIvl_spline, NULL);

	string str = seriesArray.at(data) + " " + modelArray.at(data) + " " + to_string(speed) + " " + "CL";
	const char* basicModel = str.c_str();

	worksheet_write_string(HI_vl_sheet, row, 0, manufacturerX, NULL);
	worksheet_write_string(HI_vl_sheet, row, 1, brandx, NULL);
	worksheet_write_string(HI_vl_sheet, row, 2, basicModel, NULL);
	worksheet_write_string(HI_vl_sheet, row, 3, basicModel, NULL);
	worksheet_write_string(HI_vl_sheet, row, 4, pType, NULL);
	worksheet_write_string(HI_vl_sheet, row, 5, pumpConfig, NULL);
	worksheet_write_number(HI_vl_sheet, row, 6, impDiameter, NULL);
	worksheet_write_number(HI_vl_sheet, row, 7, 7, NULL);
	worksheet_write_number(HI_vl_sheet, row, 8, stages, NULL);
	worksheet_write_number(HI_vl_sheet, row, 9, nomSpeed, NULL);
	worksheet_write_number(HI_vl_sheet, row, 13, result.motorHP, NULL);
	worksheet_write_string(HI_vl_sheet, row, 14, temp, NULL);
	worksheet_write_number(HI_vl_sheet, row, 15, flowBEP, NULL);
	worksheet_write_number(HI_vl_sheet, row, 20, hBEP, NULL);
	worksheet_write_number(HI_vl_sheet, row, 30, cip[0], NULL); //25% Control input power
	worksheet_write_number(HI_vl_sheet, row, 31, cip[1], NULL); //50%
	worksheet_write_number(HI_vl_sheet, row, 32, cip[2], NULL); //75%
	worksheet_write_number(HI_vl_sheet, row, 33, cip[3], NULL); //BEP
	if (runOut) {
		worksheet_write_number(HI_vl_sheet, row, 37, h65, NULL);
		worksheet_write_number(HI_vl_sheet, row, 38, h90, NULL);
	}
	else {
		worksheet_write_number(HI_vl_sheet, row, 35, h75, NULL);
		worksheet_write_number(HI_vl_sheet, row, 36, h110, NULL);
	}
	worksheet_write_number(HI_vl_sheet, row, 34, PEIvl_spline, NULL);
	worksheet_write_number(HI_vl_sheet, row, 39, labNumber, NULL);
}

int dataClose() {
	return 0;
}