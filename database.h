#ifndef DATABASE_H_
#define DATABASE_H_

void writeData_cl(int data, int row);
void writeData_vl(int data, int row);
void dataInit(const char* fileName);
int dataClose();

#endif