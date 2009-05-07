/**********************************************************************

 FILENAME:     uiuc_2DdataFileReader.cpp

----------------------------------------------------------------------

 DESCRIPTION:  Reads name of data file to be opened and reads data 
               into appropriate arrays or matrices

----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      02/29/2000   initial release
               10/25/2001   (RD) Modified so that it recognizes a
	                    blank line
               06/30/2003   (RD) replaced istrstream with istringstream
                            to get rid of the annoying warning about
                            using the strstream header

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         <jscott@mail.com>
               Robert Deters      <rdeters@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       -2D file name
               -conversion factor for x data
               -conversion factor for y data
               -conversion factor for z data

----------------------------------------------------------------------

 OUTPUTS:      -2D array of x data for each y case
               -1D array of y data
               -2D array of z data for each y case
               -1D array of max number of x-z data sets for each y case
               -max number of y data sets

----------------------------------------------------------------------

 CALLED BY:    uiuc_menu.cpp

----------------------------------------------------------------------

 CALLS TO:     specified 2D data file

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

#include "uiuc_2DdataFileReader.h"


void uiuc_2DdataFileReader( string file_name, 
                            double x[100][100], 
                            double y[100], 
                            double z[100][100], 
                            int xmax[100], 
                            int &ymax)
{
  ParseFile *matrix;
  double token_value1;
  double token_value2;
  int counter_y = 1, counter_x = 1;
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

      //reset token_value1 and token_value2 for first if statement
      token_value1 = -999;
      token_value2 = -999;

      token1 >> token_value1;
      token2 >> token_value2;

      //chenk to see if it is a blank line
      if (token_value1==-999 && token_value2==-999);
      //check to see if only one value on line (token2 blank)
      else if (token_value2 == -999)
        {
          y[counter_y] = token_value1 * convert_y;

          ymax = counter_y;
          counter_y++;
          counter_x = 1;
        }
      else
        {
          x[counter_y-1][counter_x] = token_value1 * convert_x;
          z[counter_y-1][counter_x] = token_value2 * convert_z;

          xmax[counter_y-1] = counter_x;
          counter_x++;
        }
    }
  delete matrix;
  return;
}

// end uiuc_2DdataFileReader.cpp
