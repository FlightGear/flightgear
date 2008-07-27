#ifndef _1D_DATA_FILE_READER_H_
#define _1D_DATA_FILE_READER_H_

#include <simgear/compiler.h>

//#include STL_STRSTREAM
#include <sstream>

#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"
#include "uiuc_warnings_errors.h"

//using std::istrstream;

int uiuc_1DdataFileReader( string file_name, 
                            double x[], 
                            double y[], 
                            int &xmax );
int uiuc_1DdataFileReader( string file_name, 
			   double x[], 
			   int y[], 
			   int &xmax );

#endif // _1D_DATA_FILE_READER_H_
