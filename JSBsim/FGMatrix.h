/*******************************************************************************

Header: FGMatrix.h
Author: Tony Peden [formatted here by Jon Berndt]
Date started: Unknown

HISTORY
--------------------------------------------------------------------------------
??/??/??   TP   Created

/*******************************************************************************
SENTRY
*******************************************************************************/

#ifndef FGMATRIX_H
#define FGMATRIX_H

/*******************************************************************************
INCLUDES
*******************************************************************************/

#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>

/*******************************************************************************
DEFINES
*******************************************************************************/


/*******************************************************************************
CONSTANTS
*******************************************************************************/


/*******************************************************************************
TYPEDEFS
*******************************************************************************/


/*******************************************************************************
GLOBALS
*******************************************************************************/

class FGMatrix
{
private:
  double **data;
  unsigned rows,cols;
  bool keep;
  char delim;
  int width,prec,origin;
  void TransposeSquare(void);
  void TransposeNonSquare(void);

public:
  FGMatrix(unsigned rows, unsigned cols);
  FGMatrix(const FGMatrix& A);
  ~FGMatrix(void);

  FGMatrix& FGMatrix::operator=(const FGMatrix& A);
  double& FGMatrix::operator()(unsigned row, unsigned col);

  unsigned FGMatrix::Rows(void) const;
  unsigned FGMatrix::Cols(void) const;

  void FGMatrix::T(void);
  void InitMatrix(void);
  void InitMatrix(double value);

  friend FGMatrix operator-(FGMatrix& A, FGMatrix& B);
  friend FGMatrix operator+(FGMatrix& A, FGMatrix& B);
  friend FGMatrix operator*(double scalar,FGMatrix& A);
  friend FGMatrix operator*(FGMatrix& Left, FGMatrix& Right);
  friend FGMatrix operator/(FGMatrix& A, double scalar);

  friend void operator-=(FGMatrix &A,FGMatrix &B);
  friend void operator+=(FGMatrix &A,FGMatrix &B);
  friend void operator*=(FGMatrix &A,FGMatrix &B);
  friend void operator*=(FGMatrix &A,double scalar);
  friend void operator/=(FGMatrix &A,double scalar);

  void SetOParams(char delim,int width,int prec, int origin=0);
};

class FGColumnVector : public FGMatrix
{
public:
  FGColumnVector(void);
  FGColumnVector(int m);
  FGColumnVector(FGColumnVector& b);
  ~FGColumnVector();

  double& operator()(int m);
};

/******************************************************************************/
#endif
