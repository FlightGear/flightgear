/*******************************************************************************

Module: FGMatrix.cpp
Author: Tony Peden [formatted here by JSB]
Date started: ??
Purpose: FGMatrix class
Called by: Various

FUNCTIONAL DESCRIPTION
--------------------------------------------------------------------------------


ARGUMENTS
--------------------------------------------------------------------------------


HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created

********************************************************************************
INCLUDES
*******************************************************************************/

#include <stdlib.h>
#include "FGMatrix.h"
#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>

/*******************************************************************************
DEFINES
*******************************************************************************/

#pragma warn -use

/*******************************************************************************
CONSTANTS
*******************************************************************************/


/*******************************************************************************
TYPEDEFS
*******************************************************************************/


/*******************************************************************************
GLOBALS
*******************************************************************************/


/*******************************************************************************
************************************ CODE **************************************
*******************************************************************************/

double** alloc(int rows,int cols)
{
  double **A;

  A = new double *[rows+1];
  if (!A)	return NULL;

  for (int i=0;i<=rows;i++){
    A[i]=new double[cols+1];
    if (!A[i]) return NULL;
  }
  return A;
}


void dealloc(double **A, int rows, int cols)
{
  for(int i=0;i<=rows;i++){
    delete[] A[i];
  }

  delete[] A;
}


FGMatrix::FGMatrix(unsigned rows, unsigned cols)
{
  this->rows=rows;
  this->cols=cols;
  keep=false;
  data=alloc(rows,cols);
}


FGMatrix::FGMatrix(const FGMatrix& A)
{
  data=NULL;
  *this=A;
}


FGMatrix::~FGMatrix(void)
{
  if (keep == false) {
    dealloc(data,rows,cols);
    rows=cols=0;
  }
}


FGMatrix& FGMatrix::operator=(const FGMatrix& A)
{
  if (&A != this) {
    if (data != NULL) dealloc(data,rows,cols);
    
    width  = A.width;
    prec   = A.prec;
    delim  = A.delim;
    origin = A.origin;
    rows   = A.rows;
    cols   = A.cols;
    keep   = false;
    
    if (A.keep  == true) {
      data = A.data;
    } else {
      data = alloc(rows,cols);
      for (int i=0; i<=rows; i++) {
	      for (int j=0; j<=cols; j++) {
		      data[i][j] = A.data[i][j];
	      }
      }
    }
  }
  return *this;
}


double& FGMatrix::operator()(unsigned row, unsigned col)
{
  return data[row][col];
}


unsigned FGMatrix::Rows(void) const
{
  return rows;
}


unsigned FGMatrix::Cols(void) const
{
  return cols;
}


void FGMatrix::SetOParams(char delim,int width,int prec,int origin)
{
  FGMatrix::delim=delim;
  FGMatrix::width=width;
  FGMatrix::prec=prec;
  FGMatrix::origin=origin;
}


void FGMatrix::InitMatrix(double value)
{
  if (data) {
    for (int i=0;i<=rows;i++) {
			for (int j=0;j<=cols;j++) {
    		operator()(i,j) = value;
			}
		}
	}
}


void FGMatrix::InitMatrix(void)
{
	this->InitMatrix(0);
}


FGMatrix operator-(FGMatrix& A, FGMatrix& B)
{
	if ((A.Rows() != B.Rows()) || (A.Cols() != B.Cols())) {
		cout << endl << "FGMatrix::operator-" << endl << '\t';
		cout << "Subtraction not defined for matrices of different sizes";
    cout << endl;
		exit(1);
	}

  FGMatrix Diff(A.Rows(),A.Cols());
  Diff.keep=true;
  for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++) {
			Diff(i,j)=A(i,j)-B(i,j);
		}
	}
	return Diff;
}


void operator-=(FGMatrix &A,FGMatrix &B)
{
	if ((A.Rows() != B.Rows()) || (A.Cols() != B.Cols())) {
		cout << endl << "FGMatrix::operator-" << endl << '\t';
		cout << "Subtraction not defined for matrices of different sizes";
    cout << endl;
		exit(1);
	}

  for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++) {
			A(i,j)-=B(i,j);
		}
	}
}


FGMatrix operator+(FGMatrix& A, FGMatrix& B)
{
	if ((A.Rows() != B.Rows()) || (A.Cols() != B.Cols())) {
		cout << endl << "FGMatrix::operator+" << endl << '\t';
		cout << "Addition not defined for matrices of different sizes";
    cout << endl;
		exit(1);
	}

  FGMatrix Sum(A.Rows(),A.Cols());
  Sum.keep = true;
	for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++) {
			Sum(i,j)=A(i,j)+B(i,j);
		}
	}
	return Sum;
}


void operator+=(FGMatrix &A,FGMatrix &B)
{
	if ((A.Rows() != B.Rows()) || (A.Cols() != B.Cols())) {
		cout << endl << "FGMatrix::operator+" << endl << '\t';
		cout << "Addition not defined for matrices of different sizes";
    cout << endl;
		exit(1);
	}
  for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++) {
			A(i,j)+=B(i,j);
		}
	}
}


FGMatrix operator*(double scalar,FGMatrix &A)
{
	FGMatrix Product(A.Rows(),A.Cols());
  Product.keep = true;
	for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++) {
    	Product(i,j) = scalar*A(i,j);
    }
	}
	return Product;
}


void operator*=(FGMatrix &A,double scalar)
{
	for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++) {
    	A(i,j)*=scalar;
    }
  }
}


FGMatrix operator*(FGMatrix &Left, FGMatrix &Right)
{
	if (Left.Cols() != Right.Rows()) {
		cout << endl << "FGMatrix::operator*" << endl << '\t';
		cout << "The number of rows in the right matrix must match the number";
		cout << endl << '\t' << "of columns in the left." << endl;
		cout << '\t' << "Multiplication not defined." << endl;
		exit(1);
	}

	FGMatrix Product(Left.Rows(),Right.Cols());
	Product.keep = true;
	for (int i=1;i<=Left.Rows();i++) {
		for (int j=1;j<=Right.Cols();j++)	{
    	Product(i,j) = 0;
      for (int k=1;k<=Left.Cols();k++) {
	   		Product(i,j)+=Left(i,k)*Right(k,j);
      }
		}
	}
	return Product;
}


void operator*=(FGMatrix &Left,FGMatrix &Right)
{
	if (Left.Cols() != Right.Rows()) {
		cout << endl << "FGMatrix::operator*" << endl << '\t';
		cout << "The number of rows in the right matrix must match the number";
		cout << endl << '\t' << "of columns in the left." << endl;
		cout << '\t' << "Multiplication not defined." << endl;
		exit(1);
	}

	double **prod;

		prod=alloc(Left.Rows(),Right.Cols());
		for (int i=1;i<=Left.Rows();i++) {
			for (int j=1;j<=Right.Cols();j++) {
      	prod[i][j] = 0;
				for (int k=1;k<=Left.Cols();k++) {
        	prod[i][j]+=Left(i,k)*Right(k,j);
				}
			}
		}
		dealloc(Left.data,Left.Rows(),Left.Cols());
		Left.data=prod;
		Left.cols=Right.cols;
}


FGMatrix operator/(FGMatrix& A, double scalar)
{
	FGMatrix Quot(A.Rows(),A.Cols());
	A.keep = true;
	for (int i=1;i<=A.Rows();i++) {
		for (int j=1;j<=A.Cols();j++)	{
	   	Quot(i,j)=A(i,j)/scalar;
		}
	}
	return Quot;
}


void operator/=(FGMatrix &A,double scalar)
{
	for (int i=1;i<=A.Rows();i++)	{
		for (int j=1;j<=A.Cols();j++) {
			A(i,j)/=scalar;
		}
	}
}


void FGMatrix::T(void)
{
	if (rows==cols)
		TransposeSquare();
	else
		TransposeNonSquare();
}


void FGMatrix::TransposeSquare(void)
{
	for (int i=1;i<=rows;i++) {
		for (int j=i+1;j<=cols;j++) {
			double tmp=data[i][j];
			data[i][j]=data[j][i];
 			data[j][i]=tmp;
		}
	}
}


void FGMatrix::TransposeNonSquare(void)
{
	double **tran;

	tran=alloc(rows,cols);
	for (int i=1;i<=rows;i++) {
		for (int j=1;j<=cols;j++) {
			tran[j][i]=data[i][j];
		}
	}
	dealloc(data,rows,cols);

	data=tran;
	unsigned m=rows;
	rows=cols;
	cols=m;
}


FGColumnVector::FGColumnVector(void):FGMatrix(3,1) { }
FGColumnVector::FGColumnVector(int m):FGMatrix(m,1) { }
FGColumnVector::FGColumnVector(FGColumnVector& b):FGMatrix(b) { }
FGColumnVector::~FGColumnVector() { }
double& FGColumnVector::operator()(int m)
{
	return FGMatrix::operator()(m,1);
}

