#ifndef _1D_DATA_FILE_READER_H_
#define _1D_DATA_FILE_READER_H_

#include <simgear/compiler.h>

#include STL_STRSTREAM

#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"

#if !defined (SG_HAVE_NATIVE_SGI_COMPILERS)
SG_USING_STD(istrstream);
#endif

int uiuc_1DdataFileReader( string file_name, 
                            double x[100], 
                            double y[100], 
                            int &xmax );
int uiuc_1DdataFileReader( string file_name, 
			   double x[], 
			   int y[], 
			   int &xmax );

#endif // _1D_DATA_FILE_READER_H_
