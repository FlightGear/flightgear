/* -*- Mode: C++ -*- *****************************************************
 * mymath.cc
 * Written by Durk Talsma, around 1995/1996.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 **************************************************************************/

/********************************************************************
 * This file defines a simple Vector and Matrix library. These were 
 * originally written for my (yet) unpublished planetarium / solar 
 * system simulator program. The are included here, because I have 
 * experience the code to calculate angles between vectors. I'm sure 
 * similar functions exist already somewhere else so I don't mind 
 * whether this gets eventually replaced by something more suitable
 * The functions are based on a description in Tyler. A. (1994). C++
 * Real-time 3D graphics. Sigma press, Wilmslow, England.
 * 
 * The original versions were written under windows, hence the occasional 
 *::MessageBox() statements between conditional compile statements
 *
 ********************************************************************/



#include "mymath.h"

const double PiOver180 = 1.74532925199433E-002;
const double Pix4dif3  = 4.1887902;

Vector::Vector(int size)
{
	build(size);
    for (int i = 0; i < nrData; i++)
    	data[i] = 0.00;
}

double Vector::VecLen()
{

   double length = 0;
   for (int i= 0; i < nrData; i++)
		length += powl(data[i],2);
   return sqrtl(length);
}


double VecDot(Vector first, Vector second)
{
	/*double
    	result = ((first.xVal*second.xVal) +
   		  		  (first.yVal*second.yVal) +
           		  (first.zVal*second.zVal));
    return result; */

    double result = 0;
    for (int i = 0; i < first.nrData; i++)
    	result += first.data[i] * second.data[i];

    return result;
}

Vector VecCross(Vector first, Vector second)
{

    	/*return Vec3 ( (first.yVal*second.zVal - first.zVal*second.yVal),
        			  (first.zVal*second.xVal - first.zVal*second.xVal),
                      (first.xVal*second.yVal - first.yVal*second.xVal) );
		*/
        #ifdef DEBUG
        if ( (first.nrData != 4) || (first.nrData != second.nrData) )
        {
        	::MessageBox(0, "Attempting to calculate Cross product with 2\n"
                         "unequally sized vectors in\n"
                         "Vector VecCross(Vector, Vector", "Error",
                         MB_OK);
            exit(1);
        }
        #endif
	double x = first.data[1] * second.data[2] - first.data[2]*second.data[1];
        double y = first.data[2] * second.data[0] - first.data[0]*second.data[2];
        double z = first.data[0] * second.data[1] - first.data[1]*second.data[0];
	return Vector(x,y,z);
}




Vector operator- (Vector first, Vector second)
{
	/*return Vec3( first.xVal - second.xVal,
    			 first.yVal - second.yVal,
                 first.zVal - second.zVal );
    */
	    #ifdef DEBUG
        if ( first.nrData != second.nrData )
        {
        	::MessageBox(0, "Attempting to subtract 2 \n"
                         "unequally sized vectors in\n"
                         "Vector operator-(Vector, Vector", "Error",
                         MB_OK);
            //exit(1);
            return Vector(0);
        }
        #endif
        double *temp = new double[first.nrData];
        for (int i = 0; i < first.nrData; i++)
        	temp[i] = first.data[i] - second.data[i];
        Vector result(first.nrData, temp);
        delete [] temp;
        return result;
}


Vector operator+ (Vector first, Vector second)
{
	/*return Vec3( first.xVal + second.xVal,
    			 first.yVal + second.yVal,
                 first.zVal + second.zVal );
    */
    #ifdef DEBUG
    if ( first.nrData != second.nrData )
	{
      	::MessageBox(0, "Attempting to add 2\n"
                         "unequally sized vectors in\n"
                         "Vector operator+(Vector, Vector", "Error",
                         MB_OK);
        exit(1);
    }
    #endif
    double *temp = new double[first.nrData];
    for (int i = 0; i < first.nrData; i++)
    	temp[i] = first.data[i] + second.data[i];
    Vector result(first.nrData, temp);
    delete [] temp;
    return result;
}

Vector Vector::operator +=(Vector other)
{
	#ifdef DEBUG
    if ( first.nrData != second.nrData )
    {
    	::MessageBox(0, "Attempting to add 2\n"
                         "unequally sized vectors in\n"
                         "Vector operator+(Vector, Vector", "Error",
                         MB_OK);
        exit(1);
    }
    #endif
    for (int i = 0; i < nrData; i++)
    	data[i] += other.data[i];
    return *this;
}

Vector Vector::operator -=(Vector other)
{
	#ifdef DEBUG
    if ( first.nrData != second.nrData )
    {
    	::MessageBox(0, "Attempting to add 2\n"
                         "unequally sized vectors in\n"
                         "Vector operator+(Vector, Vector", "Error",
                         MB_OK);
        exit(1);
    }
    #endif
    for (int i = 0; i < nrData; i++)
    	data[i] -= other.data[i];
    return *this;
}





Vector operator* (Matrix mx, Vector vc)
{
	int sizes[3];
    sizes[0] = vc.nrData;
    sizes[1] = mx.rows;
    sizes[2] = mx.columns;

    #ifdef DEBUG
    if ( (sizes[0] != sizes[1]) || (sizes[0] != sizes[2]) )
    {
        char buffer[50];
        sprintf(buffer, "Sizes don't match in function\n"
        				"Vector operator*(Matrix, Vector)\n"
                        "sizes are: %d, %d, %d", sizes[0], sizes[1],sizes[2]);
    	MessageBox(0, buffer, "Error", MB_OK | MB_ICONEXCLAMATION);
        exit(1);
    }
    #endif
    double* result = new double[sizes[0]];
    int col, row;

    for (col = 0; col < sizes[0]; col++)
    {
    	result[col] = 0;
        for (row = 0; row < sizes[0]; row++)
        	result[col] += vc[row] * mx[row][col];
    }
    Vector res(4, result);

   /*
    #ifdef DEBUG
    char buffer[200];
    sprintf(buffer, "return value of vector * matrix multiplication is\n"
    				"(%f, %f, %f, %f) ", result[0], result[1], result[2], result[3]);
    ::MessageBox(0, buffer, "Information", MB_OK);
    #endif
   */
	delete [] result;
    return res;
}

Vector operator*(Vector v1, Vector v2)
{
	int size1 = v1.nrData;

    #ifdef DEBUG

    int size2 = v2.nrData;
    if(size1 != size2)
    {
    	::MessageBox(0, "Vector sizes don't match in Vector operator*(Vector, Vector)",
        			"Error", MB_OK);
        exit(1);
    }
    #endif
    double *tempRes = new double[size1];
    for (int i= 0; i < size1; i++)
    	tempRes[i] = v1[i] * v2[i];
    Vector result(size1, tempRes);
    delete tempRes;
    return result;
}


Vector operator*(Vector vec, double d)
{
	double* tempRes = new double[vec.nrData];
    for (int i = 0; i < vec.nrData; i++)
    	tempRes[i] = vec[i] * d;
    Vector result(vec.nrData, tempRes);
    delete tempRes;
    return result;
}

Vector operator/(Vector vec, double d)
{
	double* tempRes = new double[vec.nrData];
    for (int i = 0; i < vec.nrData; i++)
    	tempRes[i] = vec[i] / d;
    Vector result(vec.nrData, tempRes);
    delete tempRes;
    return result;
}

ostream& operator << (ostream& os, Vector& vec)
{
  os << /*setw(4) << */vec.nrData << '\t';
    for (int i = 0; i < vec.nrData; i++)
      os /*<< setw(25) */<< vec[i] << '\t';
    return os;
}

istream& operator >> (istream& is, Vector& vec)
{
	is >> vec.nrData;
    if (vec.data)
    	delete [] vec.data;
    vec.data = new double[vec.nrData];
    for (int i = 0; i < vec.nrData; i++)
    	is >> vec.data[i];
    return is;
}

ostream& Vector::binSave(ostream& os)
{
	os.write((char*) &nrData, sizeof(int));
    os.write((char*) data, nrData* sizeof(double));
	return os;
}






/******************************************************************************
		Matrix manipulation routines
******************************************************************************/

Matrix::Matrix(int r, int c, double*dta)
{
	build(r,c);
    for (int i = 0; i < rows; i++)
    	for (int j = 0; j < columns; j++)
        	data[i][j] = (*dta++);
}

Matrix::Matrix(int r, int c, double** dta)
{
	build(r,c);
    for (int i = 0; i < rows; i++)
    	for (int j = 0; j < columns; j++)
			data[i][j] = dta[i][j];
}

Matrix::Matrix(int r, int c, Vector* dta)
{
	build(r,c);
    for (int i = 0; i < rows; i++)
    	data[i] = dta[i];
}

Matrix::Matrix(Matrix& other)
{
	build (other.rows, other.columns);
    for (int i = 0; i< rows; i++)
    	(*this)[i] = other[i];
}

void Matrix::build(int row, int col)
{
    rows = row;
    columns = col;

	data = new Vector [rows];
    for (int i = 0; i < rows; i++)
    	data[i].build(col);
}

Matrix& Matrix::operator =(Matrix& other)
{
	rows = other.rows;
    columns = other.columns;
    for (int i = 0; i < rows; i++)
    	(*this)[i] = other[i];
    return *this;
}


Vector Matrix::operator ()(int col)
{
	Vector Col(rows);
    for (int i = 0; i < rows; i++)
    	Col[i] = data[i][col];
    return Col;
}

void Matrix::norm(int scale)
{
	for (int i = 0; i < rows; i++)
    	for (int j = 0; j < columns; j++)
        	data[i][j] /= scale;
}
