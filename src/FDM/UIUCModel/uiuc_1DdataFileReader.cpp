/**********************************************************************

 FILENAME:     uiuc_1DdataFileReader.cpp

----------------------------------------------------------------------

 DESCRIPTION:  Reads name of data file to be opened and reads data 
               into appropriate arrays or matrices

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      02/15/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -1D data file name
               -conversion factor for y data
               -conversion factor for x data

----------------------------------------------------------------------

 OUTPUTS:      -array of x data
               -array of y data
               -max number of data sets

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu.cpp

----------------------------------------------------------------------

 CALLS TO:     specified 1D data file

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2000 by Michael Selig

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 USA or view http://www.gnu.org/copyleft/gpl.html.

**********************************************************************/

#include "uiuc_1DdataFileReader.h"

int 
uiuc_1DdataFileReader( string file_name,  
                         double x[100], double y[100], int &xmax ) 
{

  ParseFile *matrix;
  double token_value1;
  double token_value2;
  int counter = 1, data = 0;
  string linetoken1; 
  string linetoken2; 
  stack command_list;

  /* Read the file and get the list of commands */
  matrix = new ParseFile(file_name);
  command_list = matrix -> getCommands();

  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = matrix -> getToken(*command_line, 1); // gettoken(string,tokenNo);
      linetoken2 = matrix -> getToken(*command_line, 2); // 2 represents token No 2

      istrstream token1(linetoken1.c_str());
      istrstream token2(linetoken2.c_str());

      token1 >> token_value1;
      token2 >> token_value2;

      x[counter] = token_value1 * convert_x;
      y[counter] = token_value2 * convert_y;
      xmax = counter;
      counter++;
      data = 1;
    }
  return data;
}

//can't have conversions in this version since the numbers are
//to stay as integers
int 
uiuc_1DdataFileReader( string file_name,  
                         double x[], int y[], int &xmax ) 
{

  ParseFile *matrix;
  int token_value1;
  int token_value2;
  int counter = 1, data = 0;
  string linetoken1; 
  string linetoken2; 
  stack command_list;

  /* Read the file and get the list of commands */
  matrix = new ParseFile(file_name);
  command_list = matrix -> getCommands();

  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = matrix -> getToken(*command_line, 1); // gettoken(string,tokenNo);
      linetoken2 = matrix -> getToken(*command_line, 2); // 2 represents token No 2

      istrstream token1(linetoken1.c_str());
      istrstream token2(linetoken2.c_str());

      token1 >> token_value1;
      token2 >> token_value2;

      x[counter] = token_value1;
      y[counter] = token_value2;
      xmax = counter;
      counter++;
      data = 1;
    }
  return data;
}

// end uiuc_1DdataFileReader.cpp
