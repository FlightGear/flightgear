#ifndef _1D_DATA_FILE_READER_H_
#define _1D_DATA_FILE_READER_H_

#include <simgear/compiler.h>

#include <strstream>

#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"

SG_USING_STD(istrstream);

int uiuc_1DdataFileReader( string file_name, 
                            double x[100], 
                            double y[100], 
                            int &xmax );

#endif // _1D_DATA_FILE_READER_H_
