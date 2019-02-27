#ifdef _MSC_VER
#include "winsvc.hpp"
#include <windows.h>
#include <stdio.h>
#include <strsafe.h>

//#include "sample.h"

enum WinServiceState {
	WinServiceState_UNKNOWN = 0,
	WinServiceState_SERVICE_STOPPED,
	WinServiceState_SERVICE_START_PENDING,
	WinServiceState_SERVICE_STOP_PENDING,
	WinServiceState_SERVICE_RUNNING,
	WinServiceState_SERVICE_CONTINUE_PENDING,
	WinServiceState_SERVICE_PAUSE_PENDING,
	WinServiceState_SERVICE_PAUSED,
	WinServiceState_COUNT,
};

enum InstalledState {
	InstalledState_Unknown = 0,
	InstalledState_Installed,
	InstalledState_NotInstalled,

};

#define SVCNAME "DCore"

static SERVICE_STATUS          s_SvcStatus;
static SERVICE_STATUS_HANDLE   s_SvcStatusHandle;
static HANDLE                  s_hSvcStopEvent = NULL;

void SvcInstall(int argc, char *argv[]);
void SvcRemove(void);


void WINAPI SvcCtrlHandler(DWORD);
void WINAPI SvcMain(int, char**);

void ReportSvcStatus(DWORD, DWORD, DWORD);
void SvcInit(int, char**);
void SvcReportEvent(LPTSTR);
LPSTR GetLastErrorText(LPSTR lpszBuf, DWORD dwSize);
InstalledState IsInstalledAsService(WinServiceState& state, char* szErr, int maxLen);


int install_win_service(char *options_str)
{
	WinServiceState state;
	char errStr[256] = "";
	InstalledState is_installed = IsInstalledAsService(state, errStr, sizeof(errStr));
	if(is_installed == 0)
		printf("Unknown, state: %i, error: %s", state, errStr);
	if (is_installed == 1)
		printf("Installed, state: %i, error: %s", state, errStr);
	if (is_installed == 2)
		printf("Not installed, state: %i, error: %s", state, errStr);
	if (lstrcmpi(argv[1], "--install-win-service") == 0) {
		SvcInstall(argc, argv);
		return 0;
	}

	if (lstrcmpi(argv[1], "--remove-win-service") == 0) {
		SvcRemove();
		return 0;
	}

	// TO_DO: Add any additional services for the process to this table.
	SERVICE_TABLE_ENTRY DispatchTable[] = {
		{ (LPSTR)SVCNAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
		{ NULL, NULL }
	};

	// This call returns when the service has stopped. 
	// The process should simply terminate when the call returns.

	if (!StartServiceCtrlDispatcher(DispatchTable)) {
		SvcReportEvent((LPTSTR)"StartServiceCtrlDispatcher");
	}
	return 0;
}

void SvcInstall(int argc, char *argv[])
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	char szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
		printf("Cannot install service (%d)\n", GetLastError());
		return;
	}

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) {
		printf("OpenSCManager failed (%d)\n", GetLastError());
		return;
	}

	// Create the service
	schService = CreateService(
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
		SVCNAME,                   // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
		SERVICE_DEMAND_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL) {
		if (GetLastError() != ERROR_SERVICE_EXISTS) {
			printf("CreateService failed (%d)\n", GetLastError());
			CloseServiceHandle(schSCManager);
			return;
		}
	}
	else printf("Service installed successfully\n");

	char serviceRegPath[MAX_PATH];
	lstrcpy(serviceRegPath, "System\\CurrentControlSet\\Services\\");
	lstrcat(serviceRegPath, SVCNAME);
	lstrcat(serviceRegPath, "\\Parameters");

	DWORD disp = 0;
	HKEY hKey = NULL;
	DWORD err = RegCreateKeyExA(HKEY_LOCAL_MACHINE, serviceRegPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
	if (err == 0) {
		RegSetValueEx(hKey, "CommandLine", 0, REG_SZ, (BYTE*)"blaaaah", 8);
		RegCloseKey(hKey);
	}

	if(schService)
		CloseServiceHandle(schService);
	if(schSCManager)
		CloseServiceHandle(schSCManager);
}

void SvcRemove(void)
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;
	SERVICE_STATUS          ssStatus;
	char                   szErr[256];

	char cmdLine[1024];
	char moduleFullPath[1024];
	DWORD err = 0;

	ZeroMemory(cmdLine, sizeof(cmdLine));
	ZeroMemory(moduleFullPath, sizeof(moduleFullPath));
	GetModuleFileName(GetModuleHandle(NULL), moduleFullPath, 1024 - 64);

	schSCManager = 
	OpenSCManager(
		NULL,                 
		NULL,                 
		SC_MANAGER_CONNECT   
		);

	if (schSCManager) {
		schService = OpenService(schSCManager, SVCNAME, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);

		if (schService) {
			// try to stop the service
			if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus)) {
				printf("Stopping %s.", SVCNAME);
				Sleep(1000);

				while (QueryServiceStatus(schService, &ssStatus)) {

					if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
						printf(".");
						Sleep(1000);
					}
					else
						break;
				}

				if (ssStatus.dwCurrentState == SERVICE_STOPPED)
					printf("\n%s stopped.\n", SVCNAME);
				else
					printf("\n%s failed to stop.\n", SVCNAME);

			}

			// now remove the service
			if (DeleteService(schService))
				printf("%s removed.\n", SVCNAME);
			else
				printf("DeleteService failed - %s\n", GetLastErrorText(szErr, 256));


			CloseServiceHandle(schService);
		} else
			printf("OpenService failed - %s\n", GetLastErrorText(szErr, 256));

		CloseServiceHandle(schSCManager);
	} else
		printf("OpenSCManager failed - %s\n", GetLastErrorText(szErr, 256));
}



void WINAPI SvcMain(int argc, char* argv[])
{
	// Register the handler function for the service

	s_SvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		SvcCtrlHandler);

	if (!s_SvcStatusHandle) {
		SvcReportEvent((char*)"RegisterServiceCtrlHandler");
		return;
	}

	// These SERVICE_STATUS members remain as set here
	s_SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	s_SvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

	// Perform service-specific initialization and work.
	SvcInit(argc, argv);
}

void SvcInit(int argc, char* argv[])
{
	// TO_DO: Declare and set any required variables.
	//   Be sure to periodically call ReportSvcStatus() with 
	//   SERVICE_START_PENDING. If initialization fails, call
	//   ReportSvcStatus with SERVICE_STOPPED.

	// Create an event. The control handler function, SvcCtrlHandler,
	// signals this event when it receives the stop control code.

	s_hSvcStopEvent = CreateEvent(
		NULL,    // default security attributes
		TRUE,    // manual reset event
		FALSE,   // not signaled
		NULL);   // no name

	if (s_hSvcStopEvent == NULL) {
		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}

	// Report running status when initialization is complete.

	ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

	// TO_DO: Perform work until service stops.
	while (1) {
		// Check whether to stop the service.

		WaitForSingleObject(s_hSvcStopEvent, INFINITE);

		ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
		return;
	}
}

void
ReportSvcStatus(
	DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint
	)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.
	s_SvcStatus.dwCurrentState = dwCurrentState;
	s_SvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	s_SvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		s_SvcStatus.dwControlsAccepted = 0;
	else s_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		s_SvcStatus.dwCheckPoint = 0;
	else s_SvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(s_SvcStatusHandle, &s_SvcStatus);
}

void WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 
	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(s_hSvcStopEvent);
		ReportSvcStatus(s_SvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

void SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCSTR lpszStrings[2];
	char Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			SVC_ERROR,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}


LPSTR GetLastErrorText(LPSTR lpszBuf, DWORD dwSize)
{
	DWORD dwRet;
	LPTSTR lpszTemp = NULL;

	dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
		NULL,
		GetLastError(),
		LANG_NEUTRAL,
		(LPSTR)&lpszTemp,
		0,
		NULL);

	// supplied buffer is not long enough
	if (!dwRet || ((long)dwSize < (long)dwRet + 14))
		lpszBuf[0] = '\0';
	else
	{
		lpszTemp[lstrlen(lpszTemp) - 2] = '\0';  //remove cr and newline character
		snprintf(lpszBuf, dwSize, "%s (0x%x)", lpszTemp, GetLastError());
	}

	if (lpszTemp)
		LocalFree((HLOCAL)lpszTemp);

	return lpszBuf;
}

InstalledState IsInstalledAsService(WinServiceState& state, char* szErr, int maxLen)
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;
	SERVICE_STATUS	ssStatus;
	DWORD err = 0;
	state = WinServiceState_UNKNOWN;
	InstalledState result = InstalledState_Unknown;

	schSCManager =
	OpenSCManager(
		NULL,
		NULL,
		SC_MANAGER_CONNECT
	);

	if (schSCManager) {

		schService = OpenService(schSCManager, SVCNAME, SERVICE_QUERY_STATUS);
		if (schService) {
			// try to stop the service			
			if (QueryServiceStatus(schService, &ssStatus)) {
				state = (WinServiceState)ssStatus.dwCurrentState;					
			}
			else {

				if (szErr) {
					GetLastErrorText(szErr, maxLen);
					printf("QueryServiceStatus failed - %s\n", szErr);
				}
			}
			CloseServiceHandle(schService);
			result = InstalledState_Installed;
		}
		else {
			err = GetLastError();
			GetLastErrorText(szErr, maxLen);
			
			switch (err) {
				case 5:	//Access denied
					printf("OpenService failed, access denied\n");
					break;
				case 0x424:
					result = InstalledState_NotInstalled;
					break;
				default:
					if (szErr)
						printf("OpenService failed - %s\n", szErr);
					break;
			}
		}

		CloseServiceHandle(schSCManager);
	}
	else {
		if (szErr) {
			GetLastErrorText(szErr, maxLen);
			printf("OpenSCManager failed - %s\n", szErr);
		}
	}
	return result;
}
#endif