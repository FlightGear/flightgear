/**********************************************************************

 FILENAME:     uiuc_parsefile.cpp

----------------------------------------------------------------------

 DESCRIPTION:  Reads the input file and stores data in a list
               gets tokens from each line of the list
              
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   

----------------------------------------------------------------------

 HISTORY:      01/30/2000   initial release

----------------------------------------------------------------------

 AUTHOR(S):    Bipin Sehgal       <bsehgal@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       *

----------------------------------------------------------------------

 OUTPUTS:      *

----------------------------------------------------------------------

 CALLED BY:    *

----------------------------------------------------------------------

 CALLS TO:     *

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


#include "uiuc_parsefile.h"


ParseFile :: ParseFile (const string fileName)
{
  file.open(fileName.c_str());
  readFile();
}

ParseFile :: ~ParseFile ()
{
  file.close();
}

void ParseFile :: removeComments(string& inputLine)
{
  int pos = inputLine.find_first_of(COMMENT);
  
  if (pos != inputLine.npos) // a "#" exists in the line 
  {
        if (inputLine.find_first_not_of(DELIMITERS) == pos)
          inputLine = ""; // Complete line a comment
        else
          inputLine = inputLine.substr(0,pos); //Truncate the comment from the line
  }
}


string ParseFile :: getToken(string inputLine, int tokenNo)
{
  int pos = 0;
  int pos1 = 0;
  int tokencounter = 0;

  while (tokencounter < tokenNo)
  {
        if ((pos1 == inputLine.npos) || (pos1 == -1) || (pos == -1) )
          return ""; //return an empty string if tokenNo exceeds the No of tokens in the line
        
        inputLine = inputLine.substr(pos1 , MAXLINE);
        pos = inputLine.find_first_not_of(DELIMITERS);
        pos1 = inputLine.find_first_of(DELIMITERS , pos);
        tokencounter ++;
  }

  if (pos1== -1 || pos == -1)
      return "";
  else
      return inputLine.substr(pos , pos1-pos); // return the desired token 
}


void ParseFile :: storeCommands(string inputLine)
{
  int pos;
  int pos1;
  int wordlength;
  string line;
 
  inputLine += " ";  // To take care of the case when last character is not a blank
  pos = inputLine.find_first_not_of(DELIMITERS);
  pos1 = inputLine.find_first_of(DELIMITERS);
  
  while ((pos != inputLine.npos) && (pos1 != inputLine.npos))
  {
        line += inputLine.substr(pos , pos1 - pos)+ " ";
        inputLine = inputLine.substr(pos1, inputLine.size()- (pos1 - pos));
        pos = inputLine.find_first_not_of(DELIMITERS);
        pos1 = inputLine.find_first_of(DELIMITERS , pos);
  }
  
  line += inputLine; // Add the last word to the line
  commands.push_back(line);
}

void ParseFile :: readFile()
{
  string line;

  while (getline(file , line))
  {
        removeComments(line);
        if (line.find_first_not_of(DELIMITERS) != line.npos) // strip off blank lines
          storeCommands(line);
  }
}

stack ParseFile :: getCommands()
{
  return commands;
}

//end uiuc_parsefile.cpp
