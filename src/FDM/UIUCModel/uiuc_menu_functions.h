
#ifndef _MENU_FUNCTIONS_H_
#define _MENU_FUNCTIONS_H_

#include "uiuc_aircraft.h"
#include <simgear/compiler.h>

#include <string>

void d_2_to_3( double array2D[100][100], double array3D[][100][100], int index3D);
void d_1_to_2( double array1D[100], double array2D[][100], int index2D);
void d_1_to_1( double array1[100], double array2[100] );
void i_1_to_2( int array1D[100], int array2D[][100], int index2D);
bool check_float( const std::string &token);
//bool check_float( const string &token);

#endif //_MENU_FUNCTIONS_H_
