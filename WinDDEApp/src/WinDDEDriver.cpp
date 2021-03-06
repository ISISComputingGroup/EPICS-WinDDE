/*************************************************************************\
* Copyright (c) 2018 Science and Technology Facilities Council (STFC), GB.
* All rights reverved.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE.txt that is included with this distribution.
\*************************************************************************/

#include <windows.h>
#include <ddeml.h>

#include <stdlib.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <process.h>

#include <shareLib.h>
#include <epicsTypes.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <epicsString.h>
#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>
#include <epicsExit.h>
#include <errlog.h>
#include <iocsh.h>

#include "asynPortDriver.h"

#include <epicsExport.h>

#include "WinDDEDriver.h"
#include "WinDDEMsg.h"

/// to anable debug output from EPICS
static int debugDDE = 0;

/// private windows message for notifying DDE of epics value change
#define WM_EPICSDDE WM_USER

/// callback function type passed in wParam to message pump via WM_EPICSDDE
typedef void (*dde_cb_func_t)(LPARAM);

/// convert DDL string to STL string
static std::string getString(DWORD idInst, HSZ str)
{
	char* buffer = NULL;
	if (str == NULL)
	{
		return "NULL";
	}
	DWORD len = DdeQueryString(idInst, str, (LPSTR)NULL, 0, CP_WINANSI) + 1;
	buffer = new char[len];
	DdeQueryString(idInst, str, buffer, len, CP_WINANSI);
	std::string retstr(buffer);
	delete[] buffer;
	return retstr;
}

/// callback function for all DDE events
HDDEDATA CALLBACK WinDDEDriver::DdeCallback(
	UINT      uType,
	UINT      uFmt,
	HCONV     hconv,
	HSZ       hsz1,
	HSZ       hsz2,
	HDDEDATA  hdata,
	ULONG_PTR dwData1,
	ULONG_PTR dwData2
)
{
	static HSZ sysTopic = DdeCreateStringHandle(m_idInst, SZDDESYS_TOPIC, CP_WINANSI);
	//	static HSZ sysItemStatus = DdeCreateStringHandle(m_idInst, SZDDESYS_ITEM_STATUS, CP_WINANSI);
	static HSZ sysItemList = DdeCreateStringHandle(m_idInst, SZDDESYS_ITEM_SYSITEMS, CP_WINANSI);
	static HSZ sysTopicList = DdeCreateStringHandle(m_idInst, SZDDESYS_ITEM_TOPICS, CP_WINANSI);
	static HSZ topicItemList = DdeCreateStringHandle(m_idInst, SZDDE_ITEM_ITEMLIST, CP_WINANSI);
	static HSZ topicFormats = DdeCreateStringHandle(m_idInst, SZDDESYS_ITEM_FORMATS, CP_WINANSI);
	int j;
	switch (uType)
	{
	case XTYP_CONNECT:
		// hsz2 is the service name, but we only register one service 
//		   printf("connect\n");
		if (DdeCmpStringHandles(hsz1, sysTopic) == 0 || DdeCmpStringHandles(hsz1, m_pvTopic) == 0)
		{
			return (HDDEDATA)TRUE;
		}
		return (HDDEDATA)FALSE;
		break;

	case XTYP_WILDCONNECT:
		// hsz1 is topic, hsz2 is service
//		   printf("wild connect\n");
		HSZPAIR ahszp[3];
		j = 0;
		if (hsz1 == NULL || DdeCmpStringHandles(hsz1, sysTopic) == 0)
		{
			ahszp[j].hszSvc = m_serviceName;
			ahszp[j++].hszTopic = sysTopic;
		}
		if (hsz1 == NULL || DdeCmpStringHandles(hsz1, m_pvTopic) == 0)
		{
			ahszp[j].hszSvc = m_serviceName;
			ahszp[j++].hszTopic = m_pvTopic;
		}
		ahszp[j].hszSvc = NULL;
		ahszp[j++].hszTopic = NULL;
		return DdeCreateDataHandle(
			m_idInst,
			(LPBYTE)&ahszp,
			sizeof(HSZPAIR) * j,
			0,
			(HSZ)NULL,
			0,
			0);
		break;

	case XTYP_ADVREQ: // in response to DdePostAdvise()
	case XTYP_REQUEST:
		//		  printf("request\n");
		if (hsz1 == sysTopic && uFmt == CF_TEXT)
		{
			std::string itemName = getString(m_idInst, hsz2);
			std::string itemValue;
			if (debugDDE)
			{
			    std::cout << "DDE REQUEST(sysTopic): " << itemName << std::endl;
			}
			if (hsz2 == topicFormats)
			{
				itemValue = "TEXT";
			}
			else if (hsz2 == sysTopicList)
			{
				itemValue = SZDDESYS_TOPIC "\t" "getPV";
			}
			else if (hsz2 == sysItemList || hsz2 == topicItemList)
			{
				itemValue = SZDDESYS_ITEM_TOPICS "\t" SZDDESYS_ITEM_SYSITEMS "\t" SZDDE_ITEM_ITEMLIST "\t" SZDDESYS_ITEM_FORMATS;
			}
			else
			{
				//	              printf("callback %d\n", uType);
				return (HDDEDATA)NULL;
			}
			return DdeCreateDataHandle(
				m_idInst,
				(LPBYTE)itemValue.c_str(),
				itemValue.size() + 1,
				0,
				hsz2,
				CF_TEXT,
				0);
		}
		else if (hsz1 == m_pvTopic && uFmt == CF_TEXT)
		{
			std::string itemName = getString(m_idInst, hsz2);
			std::string itemValue;
			if (hsz2 == topicItemList)
			{
				itemValue = SZDDE_ITEM_ITEMLIST "\t" SZDDESYS_ITEM_FORMATS ; 
				itemValue += m_ddeItemList;
			}
			else if (hsz2 == topicFormats)
			{
				itemValue = "TEXT";
			}
			else
			{
				m_driver->getParamValueAsString(itemName, itemValue);
			}
			if (debugDDE)
			{
				std::cout << "DDE REQUEST(getPVTopic): " << itemName << " = \"" << itemValue << "\"" << std::endl;
			}
			return DdeCreateDataHandle(
				m_idInst,
				(LPBYTE)itemValue.c_str(),
				itemValue.size() + 1,
				0,
				hsz2,
				CF_TEXT,
				0);
		}
		else
		{
			std::cout << "DDE ERROR: unknown request callback type " << uType << " or format " << uFmt << std::endl;
		}
		return (HDDEDATA)NULL;
		break;

	case XTYP_POKE:
		if (hsz1 == m_pvTopic && uFmt == CF_TEXT)
		{
			std::string itemName = getString(m_idInst, hsz2);
			DWORD cbDataLen = 0;
			LPBYTE lpData;
			lpData = DdeAccessData(hdata, &cbDataLen);
			if (lpData == NULL)
			{
				return (HDDEDATA)DDE_FNOTPROCESSED;
			}
			char* buffer = new char[cbDataLen + 1]; // it should already be NULL terminated, but no harm being extra safe
			memcpy(buffer, lpData, cbDataLen);
			buffer[cbDataLen] = '\0';
			DdeUnaccessData(hdata);
			if (debugDDE)
			{
			    std::cout << "DDE POKE: " << itemName << " = \"" << buffer << "\"" << std::endl;
			}
			m_driver->setParamValueAsString(itemName, buffer);
			delete[] buffer;
			return (HDDEDATA)DDE_FACK;
		}
		else
		{
			std::cout << "DDE ERROR: unknown poke callback type " << uType << " or format " << uFmt << std::endl;
		}
		return (HDDEDATA)DDE_FNOTPROCESSED;
		break;

	case XTYP_ADVSTART:
		if (hsz1 == m_pvTopic && uFmt == CF_TEXT)
		{
			if (debugDDE)
			{
			    std::string itemName = getString(m_idInst, hsz2);
				std::cout << "DDE: advice loop: " << itemName << std::endl;
			}
			return (HDDEDATA)TRUE; // we allow advice loops on all items
		}
		else
		{
			std::string topicName = getString(m_idInst, hsz1);
			std::string itemName = getString(m_idInst, hsz2);
// we seem to get these from excel anyway
//			std::cout << "DDE ERROR: unknown advstart: topic: " << topicName << " format: " << uFmt << " item: " << itemName << std::endl;
			return (HDDEDATA)FALSE;
		}
		break;

	case XTYP_ADVSTOP:
		return (HDDEDATA)TRUE;
		break;

	default:
		printf("DDE ERROR: unknown callback %d\n", uType);
		return (HDDEDATA)NULL;
		break;
	}
	return 0;
}

void WinDDEDriver::setParamValueAsString(const std::string& itemName, const char* itemValue)
{
	int index = -1;
	asynParamType paramType;
	lock();
	if (findParam(itemName.c_str(), &index) == asynSuccess && getParamType(index, &paramType) == asynSuccess)
	{
		if (debugDDE)
		{
			std::cout << "DDE: setParamValueAsString: " << itemName << " type: " << paramType << " value: " << itemValue << std::endl;
		}
		switch (paramType)
		{
		case asynParamInt32:
			setIntegerParam(DDEIntType, index, atoi(itemValue));
			callParamCallbacks(DDEIntType);
			break;

		case asynParamFloat64:
			setDoubleParam(DDEDoubleType, index, atof(itemValue));
			callParamCallbacks(DDEDoubleType);
			break;

		case asynParamOctet:
			setStringParam(DDEStringType, index, itemValue);
			callParamCallbacks(DDEStringType);
			break;

		default:
			printf("invalid parameter type\n");
			index = -1;
			break;
		}
	}
	else
	{
		printf("unknown item %s\n", itemName.c_str());
		index = -1;
	}
	unlock();
	// post an update so all DDE clients see change
	if (index >= 0)
	{
		PostThreadMessage(m_ddeThreadId, WM_EPICSDDE, (WPARAM)(dde_cb_func_t)&WinDDEDriver::postAdvise, index);
	}
}

void WinDDEDriver::getParamValueAsString(const std::string& itemName, std::string& itemValue)
{
	int index = -1;
	asynParamType paramType;
	char buffer[32];
	lock();
	if (findParam(itemName.c_str(), &index) == asynSuccess && getParamType(index, &paramType) == asynSuccess)
	{
		switch (paramType)
		{
		case asynParamInt32:
		{
			int val;
			getIntegerParam(DDEIntType, index, &val);
			epicsSnprintf(buffer, sizeof(buffer), "%d", val);
			itemValue = buffer;
			break;
		}

		case asynParamFloat64:
		{
			double val;
			getDoubleParam(DDEDoubleType, index, &val);
			epicsSnprintf(buffer, sizeof(buffer), "%g", val);
			itemValue = buffer;
			break;
		}

		case asynParamOctet:
			getStringParam(DDEStringType, index, itemValue);
			break;

		default:
			printf("invalid parameter type\n");
			break;
		}
	}
	else
	{
		printf("unknown item %s\n", itemName.c_str());
		itemValue = "unknown";
	}
	unlock();
}

void WinDDEDriver::postAdvise(LPARAM param_id)
{
	if ( (param_id >= 0) && (DdePostAdvise(m_idInst, m_pvTopic, m_ddeItemHandles[param_id]) == 0) )
	{
		int ret = DdeGetLastError(m_idInst);
		printf("DdePostAdvise error %d\n", ret);
	}
}

static const char* driverName = "WinDDEDriver";

/// EPICS driver report function for iocsh dbior command
void WinDDEDriver::report(FILE* fp, int details)
{
	asynPortDriver::report(fp, details);
}

WinDDEDriver::WinDDEDriver(const char *portName, const char* serviceName, int options, int pollPeriod)
	: asynPortDriver(portName,
		5, /* maxAddr */
		asynInt32Mask | asynFloat64Mask | asynOctetMask | asynDrvUserMask, /* Interface mask */
		asynInt32Mask | asynFloat64Mask | asynOctetMask,  /* Interrupt mask */
		ASYN_CANBLOCK, /* asynFlags.  This driver can block but it is not multi-device */
		1, /* Autoconnect */
		0, /* Default priority */
		0)	/* Default stack size*/
{
	const char *functionName = "WinDDEDriver";
	m_ddeThreadId = GetCurrentThreadId();
	epicsAtExit(epicsExitFunc, this);
	m_idInst = 0;
	m_driver = this;
	int ret = DdeInitialize(&m_idInst, DdeCallback, APPCLASS_STANDARD | CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_EXECUTES, 0);
	if (ret != DMLERR_NO_ERROR)
	{
		printf("%s:%s: DDE error %d\n", driverName, functionName, ret);
		return;
	}
	m_serviceName = DdeCreateStringHandle(m_idInst, serviceName, CP_WINANSI);
	m_pvTopic = DdeCreateStringHandle(m_idInst, "getPV", CP_WINANSI);
	if (DdeNameService(m_idInst, m_serviceName, 0, DNS_REGISTER) == 0)
	{
		ret = DdeGetLastError(m_idInst);
		printf("%s:%s: DDE error %d\n", driverName, functionName, ret);
		return;
	}

	// Create the thread for background tasks (not used at present, could be used for I/O intr scanning) 
	if (epicsThreadCreate("WinDDETask",
		epicsThreadPriorityMedium,
		epicsThreadGetStackSize(epicsThreadStackMedium),
		(EPICSTHREADFUNC)WinDDETaskC, this) == 0)
	{
		printf("%s:%s: epicsThreadCreate failure\n", driverName, functionName);
		return;
	}
}

asynStatus WinDDEDriver::drvUserCreate(asynUser *pasynUser, const char *drvInfo, const char **pptypeName, size_t *psize)
{
	static const char *functionName = "drvUserCreate";
	asynStatus status;
	int index, addr;

	status = getAddress(pasynUser, &addr);  // addr is DDEDataType
	if (status != asynSuccess) return(status);
	status = this->findParam(drvInfo, &index);
	if (status) {
		switch (addr)
		{
		case DDEIntType:
			status = createParam(drvInfo, asynParamInt32, &index);
			break;

		case DDEDoubleType:
			status = createParam(drvInfo, asynParamFloat64, &index);
			break;

		case DDEStringType:
			status = createParam(drvInfo, asynParamOctet, &index);
			break;

		default:
			return(asynError);
			break;
		}
		m_ddeItemList += "\t";
		m_ddeItemList += drvInfo;
		m_ddeItemHandles[index] = DdeCreateStringHandle(m_idInst, drvInfo, CP_WINANSI);
	}
	pasynUser->reason = index;
	asynPrint(pasynUser, ASYN_TRACE_FLOW,
		"%s:%s: drvInfo=%s, index=%d\n",
		driverName, functionName, drvInfo, index);
	return(asynSuccess);
}

void WinDDEDriver::epicsExitFunc(void* arg)
{
	WinDDEDriver* driver = (WinDDEDriver*)arg;
	DdeNameService(m_idInst, 0, 0, DNS_UNREGISTER);
	DdeUninitialize(m_idInst);
	m_idInst = 0;
}

void WinDDEDriver::WinDDETaskC(void* arg)
{
	WinDDEDriver* driver = (WinDDEDriver*)arg;
	driver->WinDDETask();
}

void WinDDEDriver::WinDDETask()
{
}

asynStatus WinDDEDriver::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
	static const char *functionName = "writeInt32";
	int function = pasynUser->reason;
	const char *paramName = NULL;
	getParamName(function, &paramName);
	asynPortDriver::writeInt32(pasynUser, value);
	if (PostThreadMessage(m_ddeThreadId, WM_EPICSDDE, (WPARAM)(dde_cb_func_t)&WinDDEDriver::postAdvise, function) == 0)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
			"%s:%s: function=%d, name=%s, error=%d",
			driverName, functionName, function, paramName, GetLastError());
		return asynError;
	}
	return(asynSuccess);
}

asynStatus WinDDEDriver::writeFloat64(asynUser *pasynUser, epicsFloat64 value)
{
	static const char *functionName = "writeFloat64";
	int function = pasynUser->reason;
	const char *paramName = NULL;
	getParamName(function, &paramName);
	asynPortDriver::writeFloat64(pasynUser, value);
	if (PostThreadMessage(m_ddeThreadId, WM_EPICSDDE, (WPARAM)(dde_cb_func_t)&WinDDEDriver::postAdvise, function) == 0)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
			"%s:%s: function=%d, name=%s, error=%d",
			driverName, functionName, function, paramName, GetLastError());
		return asynError;
	}
	return(asynSuccess);
}

asynStatus WinDDEDriver::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual)
{
	static const char *functionName = "writeOctet";
	int function = pasynUser->reason;
	const char *paramName = NULL;
	getParamName(function, &paramName);
	asynPortDriver::writeOctet(pasynUser, value, maxChars, nActual);
	if (PostThreadMessage(m_ddeThreadId, WM_EPICSDDE, (WPARAM)(dde_cb_func_t)&WinDDEDriver::postAdvise, function) == 0)
	{
		epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
			"%s:%s: function=%d, name=%s, error=%d",
			driverName, functionName, function, paramName, GetLastError());
		return asynError;
	}
	return(asynSuccess);
}

extern "C" {

	/// @param[in] portName @copydoc initArg0
	/// @param[in] serviceName @copydoc initArg1
	int WinDDEConfigure(const char *portName, const char* serviceName, int options, int pollPeriod)
	{
		new WinDDEDriver(portName, serviceName, options, pollPeriod);
		return(asynSuccess);
	}

	// EPICS iocsh shell commands 

	static const iocshArg initArg0 = { "portName", iocshArgString };			///< The name of the asyn driver port we will create
	static const iocshArg initArg1 = { "serviceName", iocshArgString };			///< serviceName
	static const iocshArg initArg2 = { "options", iocshArgInt };
	static const iocshArg initArg3 = { "pollPeriod", iocshArgInt };

	static const iocshArg * const initArgs[] = { &initArg0,
		&initArg1,
		&initArg2,
		&initArg3 };

	static const iocshFuncDef initFuncDef = { "WinDDEConfigure", sizeof(initArgs) / sizeof(iocshArg*), initArgs };

	static void initCallFunc(const iocshArgBuf *args)
	{
		WinDDEConfigure(args[0].sval, args[1].sval, args[2].ival, args[3].ival);
	}

	/// Register new commands with EPICS IOC shell
	static void WinDDERegister(void)
	{
		iocshRegister(&initFuncDef, initCallFunc);
	}

	epicsExportRegistrar(WinDDERegister);
	epicsExportAddress(int, debugDDE);

}

// this is modified from the usual epics ioc "main"
// we need to keep the main thread to run a message loop 
// for the DDE messages as it has already called DdeInitialize()
//
// Any IOC wanting to be a DDE server will need to use this as
// its main program to run a message pump
static void iocMain(void* arg)
{
	DWORD mainThreadId = (DWORD)arg;
	iocsh(NULL);
	PostThreadMessage(mainThreadId, WM_QUIT, 0, 0); // so message loop terminates and exit called from main thread
}

epicsShareExtern void WinDDEIOCMain()
{	
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
}

DWORD WinDDEDriver::m_idInst = 0;
HSZ WinDDEDriver::m_serviceName = NULL;
DWORD WinDDEDriver::m_ddeThreadId; ///< id of thread used to access ddeml 
std::map<int, HSZ> WinDDEDriver::m_ddeItemHandles;
HSZ WinDDEDriver::m_pvTopic = NULL;
WinDDEDriver* WinDDEDriver::m_driver = NULL;
std::string WinDDEDriver::m_ddeItemList;