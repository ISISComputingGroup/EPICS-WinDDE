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

// include definition of private windows message and callback function types  
#include "WinDDEMsg.h"

// this is modified from the usual epics ioc "main"
// we need to keep the main thread to run a message loop 
// for the DDE messages as it has already called DdeInitialize()
//
// Any IOC wanting to be a DDE server will need to have
// its main program changed in a similar was to run a message pump

static void iocMain(void* arg)
{
	DWORD mainThreadId = (DWORD)arg;
	iocsh(NULL);
	PostThreadMessage(mainThreadId, WM_QUIT, 0, 0); // so message loop terminates and exit called from main thread
}

int main(int argc, char *argv[])
{
	if (argc >= 2) {
		iocsh(argv[1]);
		epicsThreadSleep(.2);
	}
	// start interactive ioc shell in separate thread to keep main thread for use as message pump
	_beginthread(iocMain, 0, (void*)GetCurrentThreadId());

	// standard windows message pump to process DDE messages
	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
			printf("win message error\n");
		}
		else
		{
			if (msg.message == WM_EPICSDDE) // special message used to enabel us to notify DDE of parameter changes
			{
				(*(dde_cb_func_t)msg.wParam)(msg.lParam); // will call WinDDEDriver::postAdvise() with given parameter
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	epicsExit(0);
	return(0);
}
