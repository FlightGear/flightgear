#ifndef _1D_DATA_FILE_READER_H_
#define _1D_DATA_FILE_READER_H_

#include <strstream.h>
#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"


int uiuc_1DdataFileReader( string file_name, 
			   double convert_x, 
			   double convert_y, 
			   double x[100], 
			   double y[100], 
			   int &xmax );

#endif // _1D_DATA_FILE_READER_H_
