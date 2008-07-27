#ifndef _2D_DATA_FILE_READER_H_
#define _2D_DATA_FILE_READER_H_

#include <simgear/compiler.h>

//#include STL_STRSTREAM
#include <sstream>

#include "uiuc_parsefile.h"
#include "uiuc_aircraft.h"

//using std::istrstream;

void uiuc_2DdataFileReader( string file_name, 
                            double x[100][100], 
                            double y[100], 
                            double z[100][100], 
                            int xmax[100], 
                            int &ymax);

#endif // _2D_DATA_FILE_READER_H_
