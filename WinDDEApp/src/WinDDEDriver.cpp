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
			//			  printf("sys request: %s\n", itemName.c_str());
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
			//			  printf("request: %s\n", itemName.c_str());
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
			printf("unknown request callback %d\n", uType);
		}
		return (HDDEDATA)NULL;

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
			//			  printf("poke: %s=%s\n", itemName.c_str(), buffer);
			m_driver->setParamValueAsString(itemName, buffer);
			delete[] buffer;
			return (HDDEDATA)DDE_FACK;
		}
		//	      printf("unknown poke callback %d\n", uType);
		return (HDDEDATA)DDE_FNOTPROCESSED;

	case XTYP_ADVSTART:
		if (hsz1 == m_pvTopic && uFmt == CF_TEXT)
		{
			//		      std::string itemName = getString(m_idInst, hsz2);
			//		      printf("advice loop: %s\n", itemName.c_str());
			return (HDDEDATA)TRUE; // we allow advice loops on all items
		}
		else
		{
			//		      std::string topicName = getString(m_idInst, hsz1);
			//		      std::string itemName = getString(m_idInst, hsz2);
			//	          printf("advstart callback %s %s %d\n", topicName.c_str(), itemName.c_str(), uFmt);
			return (HDDEDATA)FALSE;
		}

	case XTYP_ADVSTOP:
		return (HDDEDATA)TRUE;

	default:
		printf("unknown callback %d\n", uType);
		return (HDDEDATA)NULL;
		break;
	}
	return 0;
}

void WinDDEDriver::setParamValueAsString(const std::string& itemName, const char* itemValue)
{
	int index;
	asynParamType paramType;
	lock();
	if (findParam(itemName.c_str(), &index) == asynSuccess)
	{
		getParamType(index, &paramType);
		switch (paramType)
		{
		case asynParamInt32:
			setIntegerParam(DDEIntType, index, atoi(itemValue));
			break;

		case asynParamFloat64:
			setDoubleParam(DDEDoubleType, index, atof(itemValue));
			break;

		case asynParamOctet:
			setStringParam(DDEStringType, index, itemValue);
			break;

		default:
			printf("invalid parameter type\n");
			break;
		}
	}
	else
	{
		printf("unknown item %s\n", itemName.c_str());
	}
	callParamCallbacks();
	unlock();
	// post an update so all DDE clients see change
	PostThreadMessage(m_ddeThreadId, WM_EPICSDDE, (WPARAM)(dde_cb_func_t)&WinDDEDriver::postAdvise, index);
}

void WinDDEDriver::getParamValueAsString(const std::string& itemName, std::string& itemValue)
{
	int index;
	asynParamType paramType;
	char buffer[32];
	lock();
	if (findParam(itemName.c_str(), &index) == asynSuccess)
	{
		getParamType(index, &paramType);
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
	if (DdePostAdvise(m_idInst, m_pvTopic, m_ddeItemHandles[param_id]) == 0)
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

}

DWORD WinDDEDriver::m_idInst = 0;
HSZ WinDDEDriver::m_serviceName = NULL;
DWORD WinDDEDriver::m_ddeThreadId; ///< id of thread used to access ddeml 
std::map<int, HSZ> WinDDEDriver::m_ddeItemHandles;
HSZ WinDDEDriver::m_pvTopic = NULL;
WinDDEDriver* WinDDEDriver::m_driver = NULL;
std::string WinDDEDriver::m_ddeItemList;