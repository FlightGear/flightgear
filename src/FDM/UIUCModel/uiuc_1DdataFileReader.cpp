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
               09/01/2002   (RD) added second data file reader for
                            integer case
               06/30/2003   (RD) replaced istrstream with istringstream
                            to get rid of the annoying warning about
                            using the strstream header

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>
               Robert Deters      <rdeters@uiuc.edu>

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include "uiuc_1DdataFileReader.h"

int 
uiuc_1DdataFileReader( string file_name,  
                         double x[], double y[], int &xmax ) 
{

  ParseFile *matrix;
  double token_value1;
  double token_value2;
  int counter = 1, data = 0;
  string linetoken1; 
  string linetoken2; 
  stack command_list;
  static string uiuc_1DdataFileReader_error = " (from uiuc_1DdataFileReader.cpp) ";

  /* Read the file and get the list of commands */
  matrix = new ParseFile(file_name);
  command_list = matrix -> getCommands();

  for (LIST command_line = command_list.begin(); command_line!=command_list.end(); ++command_line)
    {
      linetoken1 = matrix -> getToken(*command_line, 1); // gettoken(string,tokenNo);
      linetoken2 = matrix -> getToken(*command_line, 2); // 2 represents token No 2

      istringstream token1(linetoken1.c_str());
      istringstream token2(linetoken2.c_str());

      token1 >> token_value1;
      token2 >> token_value2;

      x[counter] = token_value1 * convert_x;
      y[counter] = token_value2 * convert_y;
      xmax = counter;
      counter++;
      //(RD) will create error check later, we can have more than 100
      //if (counter > 100)
      //{      
      //  uiuc_warnings_errors(6, uiuc_1DdataFileReader_error);
      //};
      data = 1;
    }
  delete matrix;
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

      istringstream token1(linetoken1.c_str());
      istringstream token2(linetoken2.c_str());

      token1 >> token_value1;
      token2 >> token_value2;

      x[counter] = token_value1;
      y[counter] = token_value2;
      xmax = counter;
      counter++;
      data = 1;
    }
  delete matrix;
  return data;
}

// end uiuc_1DdataFileReader.cpp
