/* -*- Mode: C++ -*- *****************************************************
 * mymath.h
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


#ifndef _MY_MATH_H_
#define _MY_MATH_H__

#include <simgear/compiler.h>

#include <math.h>
#include STL_FSTREAM
#include STL_IOMANIP

FG_USING_NAMESPACE(std);

#include <simgear/constants.h>

extern const  double PiOver180;
extern const double Pix4dif3;



class Matrix;

class Vector
{
    private:
        int nrData;
	double* data;

	public:
    	Vector();
        Vector(int size);
        Vector(int size, double* dta);
        Vector(double x, double y, double z);
	Vector(Vector& other);
	Vector(istream& is);
        ~Vector();

	void SetVal(double x, double y, double z);

        void build(int);
	Vector* GetVector();
	double GetX();
	double GetY();
	double GetZ();
	void AddX(double x);
        void AddY(double y);
	void AddZ(double z);
	void SubtractX(double x);
	void SubtractY(double y);
	void SubtractZ(double z);
	double VecLen();
        Vector& operator = (Vector&);
        double& operator[] (int);
        int getDim();
	
        ostream& binSave(ostream& os);
	
	Vector operator +=(Vector other);
        Vector operator -=(Vector other);
	
	friend double VecDot(Vector first, Vector second);
	friend Vector VecCross(Vector first, Vector second);
	friend Vector operator-(Vector first, Vector second);
	friend Vector operator+(Vector first, Vector second);
	friend Vector operator*(Matrix mx, Vector vc);
	friend Vector operator*(Vector v1, Vector v2);
	friend Vector operator*(Vector vec, double d);
	friend Vector operator/(Vector vec, double d);
	
	friend ostream& operator << (ostream& os, Vector& vec);
	friend istream& operator >> (istream& is, Vector& vec);
};

/*-----------------------------------------------------------------------------
				nonmember friend functions
------------------------------------------------------------------------------*/

double VecDot(Vector first, Vector second);
Vector VecCross(Vector first, Vector second);
Vector operator-(Vector first, Vector second);
Vector operator+(Vector first, Vector second);
Vector operator*(Matrix mx, Vector vc);
Vector operator*(Vector v1, Vector v2);
Vector operator*(Vector vec, double d);
Vector operator/(Vector vec, double d);

ostream& operator << (ostream& os, Vector& vec);
istream& operator >> (istream& is, Vector& vec);


/*------------------------------------------------------------------------------
			inline member functions
------------------------------------------------------------------------------*/

inline Vector::Vector()
{
	nrData = 0;
    data = 0;
}

inline void Vector::build(int size)
{
	nrData = size;
    data = new double[nrData];
    #ifdef DEBUG
    if (!data)
    {
		::MessageBox(0, "Error Allocating Memory for a new Vector", "Error",
        			MB_OK);
        exit(1);
    }
    #endif
    //for (int i = 0; i < nrData; i++)
    //	data[i] = 0.00;
}


inline Vector::Vector(int size, double* dta)
{
	build(size);
    memcpy(data, dta, nrData*sizeof(double));
}


inline Vector::Vector(Vector& other)
{
	build(other.nrData);
    memcpy(data,other.data,nrData*sizeof(double));
}

inline Vector::Vector(double x, double y, double z)
{
    build(4);      // one extra for matrix multiplication...
	data[0] = x;
	data[1] = y;
	data[2] = z;
    data[3] = 0.00;

}

inline Vector::Vector(istream& is)
{
	is.read((char*) &nrData, sizeof(int));
    data = new double[nrData];
    is.read((char*) data, nrData * sizeof(double));
}

inline Vector::~Vector()
{
	delete [] data;
}


inline void Vector::SetVal(double x, double y, double z)
{
	#ifdef DEBUG
    if (nrData != 4)
    {
    	::MessageBox(0, "Attempt to assign data to a vector\n"
                        "With size unequal to 4 in function\n"
                        " Vector::Setval(double, double, double", "Error" , MB_OK);
     	exit(1);
    }
    #endif
    data[0] = x,
    data[1] = y;
    data[2] = z;
    data[3] = 0.00;
}

inline Vector* Vector::GetVector()
{
	return this;
}

inline double Vector::GetX()
{
	#ifdef DEBUG
    if (nrData < 1)
    {
    	::MessageBox(0, "Attempt to retrieve x-value of a vector\n"
                        "With size smaller than 1 in function\n"
                        " Vector::GetX();", "Error", MB_OK);
     	exit(1);
    }
    #endif
    return data[0];
}

inline double Vector::GetY()
{
	#ifdef DEBUG
    if (nrData < 2)
    {
    	::MessageBox(0, "Attempt to retrieve the y value of a vector\n"
                        "With size smaller than 2 in function\n"
                        " Vector::GetY();", "Error", MB_OK);
     	exit(1);
    }
    #endif
    return data[1];
}

inline double Vector::GetZ()
{
	#ifdef DEBUG
    if (nrData < 3)
    {
		::MessageBox(0, "Attempt to retrieve the z value of a vector\n"
                        "With size smaller than 2 in function\n"
                        " Vector::GetZ();", "Error", MB_OK);
        exit(1);
    }
    #endif
    return data[2];
}

inline void Vector::AddX(double x)
{
	#ifdef DEBUG
    if (nrData < 1)
    {
    	::MessageBox(0, "Attempt to chance x-value to a vector\n"
                        "With size smaller than 1 in function\n"
                        " Vector::AddX(double);", "Error", MB_OK);
     	exit(1);
    }
    #endif
    data[0] += x;
}

inline void Vector::AddY(double y)
{
	#ifdef DEBUG
    if (nrData < 2)
    {
    	::MessageBox(0, "Attempt to chance y-value to a vector\n"
                        "With size smaller than 2 in function\n"
                        " Vector::AddY(double);", "Error", MB_OK);
     	exit(1);
    }
    #endif
    data[1] += y;
}

inline void Vector::AddZ(double z)
{
	#ifdef DEBUG
    if (nrData < 3)
    {
    	::MessageBox(0, "Attempt to chance z-value to a vector\n"
                        "With size smaller than 3 in function\n"
                        " Vector::AddZ(double);", "Error", MB_OK);
     	exit(1);
    }
    #endif
    data[2] += z;
}

inline void Vector::SubtractX(double x)
{
	#ifdef DEBUG
    if (nrData < 1)
    {
    	::MessageBox(0, "Attempt to chance x-value to a vector\n"
                        "With size smaller than 1 in function\n"
                        " Vector::SubtractX(double);", "Error", MB_OK);
     	exit(1);
    }
    #endif
    data[0] -= x;
}

inline void Vector::SubtractY(double y)
{
	#ifdef DEBUG
    if (nrData < 2)
    {
    	::MessageBox(0, "Attempt to chance y-value to a vector\n"
                        "With size smaller than 2 in function\n"
                        " Vector::SubractY(double);", "Error", MB_OK);
     	exit(1);
    }
    #endif
    data[1] -= y;
}

inline void Vector::SubtractZ(double z)
{
	#ifdef DEBUG
    if (nrData < 3)
    {
    	::MessageBox(0, "Attempt to chance z-value to a vector\n"
                        "With size smaller than 3 in function\n"
                        " Vector::SubtractZ(double);", "Error", MB_OK);
     	exit(1);
    }
    #endif
    data[2] -= z;
}


inline Vector& Vector::operator= (Vector& other)
{
    if (data)
    	delete[] data;
    build(other.nrData);
    memcpy(data, other.data, nrData*sizeof(double));
    return *this;
}

inline double& Vector::operator [](int index)
{
	return *(data+index);
}


inline int Vector::getDim()
{
	return nrData;
}

/*-----------------------------------------------------------------------------
							Some generic conversion routines
------------------------------------------------------------------------------*/

float CosD(float angle);
float SinD(float angle);
float Radians(float angle);
int Round(float value);

/* ----------------------------------------------------------------------------
				And their inlined implementation
------------------------------------------------------------------------------*/

inline float CosD(float angle)
{
	return cos(Radians(angle));
}

inline float SinD(float angle)
{
	return(Radians(angle));
}


inline float Radians(float angle)
{
	return (angle*PiOver180);
}

inline int Round(float value)
{
	return ( (int) (value+0.5));
}



/******************************************************************************

							Matrix class

******************************************************************************/

class Matrix
{
    protected:
        int rows;
        int columns;
        Vector* data;

    public:

        Matrix();
        Matrix(int r, int c);
		Matrix(int r, int c, double* dta);
        Matrix(int r, int c, double** dta);
        Matrix(int r, int c, Vector*dta);
        Matrix(Matrix&);
        ~Matrix();

        void build(int r, int c);
        Matrix& operator=(Matrix&);
        Vector& operator[](int);
        Vector operator ()(int);

		int getrows();
        int getcols();
        void norm(int scal);

        friend Vector operator*(Matrix mc, Vector vc);
};

/*------------------------------------------------------------------------------
					inline Matrix routines
------------------------------------------------------------------------------*/

inline Matrix::Matrix()
{
	rows = 0;
    columns = 0;
    data = 0;
}

inline Matrix::Matrix(int r, int c)
{
	build(r, c);
}

inline Matrix::~Matrix()
{
	delete [] data;
}


inline Vector& Matrix::operator[] (int row)
{
	return data[row];
}


#endif // _MYMATH_H_


