/*************************************************************************\
* Copyright (c) 2018 Science and Technology Facilities Council (STFC), GB.
* All rights reverved.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE.txt that is included with this distribution.
\*************************************************************************/

#ifndef WINDDEDRIVER_H
#define WINDDEDRIVER_H

class WinDDEDriver : public asynPortDriver
{
public:
	enum { DDEIntType = 0, DDEDoubleType = 1, DDEStringType = 2 } DDEDataType;  // mapped to asyn addr parameter
	WinDDEDriver(const char *portName, const char* serviceName, int options, int pollPeriod);
	virtual asynStatus drvUserCreate(asynUser *  pasynUser, const char *  drvInfo, const char **  pptypeName, size_t *  psize);

	virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
	virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
	virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
	virtual void report(FILE* fp, int details);
	void WinDDETask();
	void getParamValueAsString(const std::string& itemName, std::string& itemValue);
	void setParamValueAsString(const std::string& itemName, const char* itemValue);
	static void epicsExitFunc(void* arg);
	static void WinDDETaskC(void* arg);
	static HDDEDATA CALLBACK DdeCallback(
		UINT      uType,
		UINT      uFmt,
		HCONV     hconv,
		HSZ       hsz1,
		HSZ       hsz2,
		HDDEDATA  hdata,
		ULONG_PTR dwData1,
		ULONG_PTR dwData2);
	static void postAdvise(LPARAM param_id); // called externally from windows message loop when WM_EPICSDDE received
	static DWORD m_idInst;
	static HSZ m_serviceName;
	static DWORD m_ddeThreadId; ///< id of thread used to initialise DDEML and which must be used for all DDEML functions
	static std::map<int, HSZ> m_ddeItemHandles;
	static std::string m_ddeItemList; ///< tab separated list of all asyn parameter items we support
	static HSZ m_pvTopic;  ///< our getPV topic name
	static WinDDEDriver* m_driver;
private:
};

#endif /* WINDDEDRIVER_H */
