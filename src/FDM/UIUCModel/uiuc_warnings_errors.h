#ifndef _WARNINGS_ERRORS_H_
#define _WARNINGS_ERRORS_H_

#include <simgear/compiler.h>	/* for FG_USING_STD */

#include <string>
#include <iostream>
#include <stdlib.h>		/* for exit */

FG_USING_STD(string);
FG_USING_STD(iostream);

void uiuc_warnings_errors(int errorCode, string line);

#endif  //_WARNINGS_ERRORS_H_
