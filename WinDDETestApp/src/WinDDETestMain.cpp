/*************************************************************************\
* Copyright (c) 2018 Science and Technology Facilities Council (STFC), GB.
* All rights reverved.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE.txt that is included with this distribution.
\*************************************************************************/

#include <windows.h>
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <process.h>

#include <epicsExit.h>
#include <epicsThread.h>
#include <iocsh.h>
#include <shareLib.h>

// include definition of WinDDEIOCMain() 
#include "WinDDEMsg.h"

int main(int argc, char *argv[])
{
	if (argc >= 2) {
		iocsh(argv[1]);
		epicsThreadSleep(.2);
	}
	
	// special IOC main that runs iocsh(NULL) in a separate thread and 
	// a windows message loop in main thread
    // Any IOC wanting to be a DDE server will need to have
    // its main program changed in a similar way to run a message pump
	WinDDEIOCMain();
	
	epicsExit(0);
	return(0);
}
