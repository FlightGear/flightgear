
#include "uiuc_menu_functions.h"

bool check_float( const string &token)
{
  float value;
  istrstream stream(token.c_str()); 
  return (stream >> value);
}

void d_2_to_3( double array2D[100][100], double array3D[][100][100], int index3D)
{
  for (register int i=0; i<=99; i++)
    {
      for (register int j=1; j<=99; j++)
	{
	  array3D[index3D][i][j]=array2D[i][j];
	}
    }
}

void d_1_to_2( double array1D[100], double array2D[][100], int index2D)
{
  for (register int i=0; i<=99; i++)
    {
      array2D[index2D][i]=array1D[i];
    }
}

void d_1_to_1( double array1[100], double array2[100] )
{
  for (register int i=0; i<=99; i++)
    {
      array2[i]=array1[i];
    }
}

void i_1_to_2( int array1D[100], int array2D[][100], int index2D)
{
  for (register int i=0; i<=99; i++)
    {
      array2D[index2D][i]=array1D[i];
    }
}
