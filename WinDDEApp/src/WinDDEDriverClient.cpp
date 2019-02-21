#include <windows.h>
#include <ddeml.h>
#include <stdio.h>
#include <process.h>

static const char szApp[] = "Laboratory";
static char szTopic[] = "DDEGet";
static const DWORD timeout_ms = 5000;
static DWORD ddeThreadId;

/// private windows message for notifying DDE of epics value change
#define WM_EPICSDDE WM_USER

DWORD idInst=0;
HSZ hszApp, hszTopic;

static HDDEDATA CALLBACK DdeCallback(
    UINT uType,     // Transaction type.
    UINT uFmt,      // Clipboard data format.
    HCONV hconv,    // Handle to the conversation.
    HSZ hsz1,       // Handle to a string.
    HSZ hsz2,       // Handle to a string.
    HDDEDATA hdata, // Handle to a global memory object.
    DWORD dwData1,  // Transaction-specific data.
    DWORD dwData2)  // Transaction-specific data.
{
    return 0;
}

static void DDEExecute(DWORD idInst, HCONV hConv, const char* szCommand)
{
    HDDEDATA hData = DdeCreateDataHandle(idInst, (LPBYTE)szCommand,
                               lstrlen(szCommand)+1, 0, NULL, CF_TEXT, 0);
    if (hData==NULL)   {
        printf("Command failed: %s error %u\n", szCommand, DdeGetLastError(idInst));
    }
    else    {
        HDDEDATA res = DdeClientTransaction((LPBYTE)hData, 0xFFFFFFFF, hConv, 0L, 0,
                             XTYP_EXECUTE, timeout_ms/*TIMEOUT_ASYNC*/, NULL);
		if (res == NULL)
		{
            printf("Command failed: %s error %u\n", szCommand, DdeGetLastError(idInst));
		}
    }
//	DdeFreeDataHandle(hData); // not needed as becomed invaliad afet  DdeClientTransaction call
}

static void DDERequest(DWORD idInst, HCONV hConv, const char* szItem, char* result, int len_result)
{
    HSZ hszItem = DdeCreateStringHandle(idInst, szItem, 0);
    HDDEDATA hData = DdeClientTransaction(NULL,0,hConv,hszItem,CF_TEXT, 
                                 XTYP_REQUEST, timeout_ms/*TIMEOUT_ASYNC*/ , NULL);
    if (hData==NULL)
    {
        printf("Request failed: %s error %u\n", szItem, DdeGetLastError(idInst));
		result[0] = '\0';
    }
    else
    {
		DWORD len = DdeGetData(hData, (unsigned char *)result, len_result-1, 0);
		result[len] = '\0';		
	    DdeFreeDataHandle(hData);
    }
}

static void dataPoller(void* arg)
{
	while(true)
	{
		Sleep(1000);
	    if (PostThreadMessage(ddeThreadId, WM_EPICSDDE, (WPARAM)0, 0) == 0)
	    {
		    printf("dataPoller error\n");
	    }
	}
}

int main(int argc, char* argv[])
{

    UINT iReturn;
	ddeThreadId = GetCurrentThreadId();

    iReturn = DdeInitialize(&idInst, (PFNCALLBACK)DdeCallback, 
                            APPCLASS_STANDARD | APPCMD_CLIENTONLY, 0 );
    if (iReturn!=DMLERR_NO_ERROR)
    {
        printf("DDE Initialization Failed: 0x%04x\n", iReturn);
        Sleep(1500);
        return 0;
    }

    HCONV hConv;
    hszApp = DdeCreateStringHandle(idInst, szApp, 0);
    hszTopic = DdeCreateStringHandle(idInst, szTopic, 0);
    hConv = DdeConnect(idInst, hszApp, hszTopic, NULL);
    DdeFreeStringHandle(idInst, hszApp);
    DdeFreeStringHandle(idInst, hszTopic);
    if (hConv == NULL)
    {
        printf("DDE Connection Failed.\n");
        Sleep(1500); 
		DdeUninitialize(idInst);
        return 0;
    }

//    DDEExecute(idInst, hConv, "Source:Instruments.Count");
    DDEExecute(idInst, hConv, "Source:Instruments(\"Humidity Controller\").Devices(\"Humidity Programmer\").Object.Temperature");
		
	char result[256], command[256];

	_beginthread(dataPoller, 0, NULL);

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
                DDEExecute(idInst, hConv, "Get");
                DDERequest(idInst, hConv, "Value", result, sizeof(result)); 
				sprintf(command, "caput IN:IMAT:HUMIDITY %s", result);
				printf("%s\n", command);
				system(command);
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
    //DDE Disconnect and Uninitialize.
    DdeDisconnect(hConv);
    DdeUninitialize(idInst);

    Sleep(3000);
    return 1;
}




