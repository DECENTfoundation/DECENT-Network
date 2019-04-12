#include <windows.h>

#include "winsvc.hpp"

#include <iostream>
#include <stdio.h>
#include <strsafe.h>
#include <shlobj.h>
#include <vector>


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


static SERVICE_STATUS          s_SvcStatus;
static SERVICE_STATUS_HANDLE   s_SvcStatusHandle;

void WINAPI SvcCtrlHandler(DWORD);
LPSTR GetLastErrorText(LPSTR lpszBuf, DWORD dwSize);

std::string GetAppDataDir()
{
	char szPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, szPath);
	return szPath;
}

bool IsRunningAsSystemService()
{
	DWORD sessionId = 0;
	ProcessIdToSessionId(GetCurrentProcessId(), &sessionId);
	if (sessionId == 0) 
		return true;
	return false;
}

DWORD install_win_service()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;
	bool installed = false;
	DWORD err = 0;
	char szPath[MAX_PATH];

	if (!GetModuleFileName(NULL, szPath, MAX_PATH)) {
      err = GetLastError();
		std::cerr << "Cannot install service, error:" << err << std::endl;
		return err;
	}

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager(
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 

	if (NULL == schSCManager) {
		err = GetLastError();
		if (err == ERROR_ACCESS_DENIED)
         std::cerr << "Insufficient rights to install service" << std::endl;
		else
         std::cerr << "OpenSCManager failed, error: " << err << std::endl;
		return err;
	}

	// Create the service
	schService = CreateService(
		schSCManager,              // SCM database 
		SVCNAME,                   // name of service 
      SVCDISPLAYNAME,            // service name to display 
		SERVICE_ALL_ACCESS,        // desired access 
		SERVICE_WIN32_OWN_PROCESS, // service type 
      SERVICE_AUTO_START,      // start type 
		SERVICE_ERROR_NORMAL,      // error control type 
		szPath,                    // path to service's binary 
		NULL,                      // no load ordering group 
		NULL,                      // no tag identifier 
		NULL,                      // no dependencies 
		NULL,                      // LocalSystem account 
		NULL);                     // no password 

	if (schService == NULL) {
		err = GetLastError();
		if (err != ERROR_SERVICE_EXISTS) {
         std::cerr << "CreateService failed, error: " << err << std::endl; 
			CloseServiceHandle(schSCManager);
			return GetLastError();
		}
		else // path can be changed
		{
			std::vector<char> buffer;
			LPQUERY_SERVICE_CONFIG pConfig;
			DWORD bytesNeeded = sizeof(QUERY_SERVICE_CONFIG);

			schService = OpenService(schSCManager, SVCNAME, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG);
			if (schService) {
				do
				{
					buffer.resize(bytesNeeded);
					pConfig = (LPQUERY_SERVICE_CONFIG)&buffer[0];

					if (QueryServiceConfig(schService, pConfig, (DWORD)buffer.size(), &bytesNeeded)) {
						err = 0;
						break;
					}
					err = GetLastError();
				} while (err == ERROR_INSUFFICIENT_BUFFER);

				if (err == ERROR_SUCCESS) {
					if (lstrcmpi(pConfig->lpBinaryPathName, szPath) != 0) {
						// stop service
						bool stopped = false;
						SERVICE_STATUS          ssStatus;
						if (ControlService(schService, SERVICE_CONTROL_STOP, &ssStatus)) {
                     std::cout << "Stopping " << SVCNAME << "." << std::endl;;
							Sleep(1000);
							DWORD counter = 0;
							while (QueryServiceStatus(schService, &ssStatus) || counter < 60) {

								if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                           std::cout << ".";
									Sleep(1000);
								}
								else
									break;
								counter++;
							}

							if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
								stopped = true;
                        std::cout << SVCNAME << " stopped." << std::endl;
							} 
							else
                        std::cout << SVCNAME << " failed to stop." << std::endl;
						}
						if (ssStatus.dwCurrentState == SERVICE_STOPPED) {
							stopped = true;
                     std::cout << SVCNAME << " stopped." << std::endl;
						}
						if (stopped) {
							if (!ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, SERVICE_NO_CHANGE, szPath, NULL, NULL, NULL, NULL, NULL, NULL)) 
							{	
								err = GetLastError();
								if (err == ERROR_ACCESS_DENIED)
                           std::cerr << "Insufficient rights to change service configuration" << std::endl;
								else
                           std::cerr << "Cannot change service config, error: " << err << std::endl;
							} else
                        installed = true;// ok, changed
						}
					} else
                  installed = true;
				} else
               std::cerr << "Cannot retrieve service config, error: " << err << std::endl;
				CloseServiceHandle(schService);
			} else {
				err = GetLastError();
            if(err == ERROR_ACCESS_DENIED)
               std::cerr << "Insufficient rights to open existing service" << std::endl;
				else 
               std::cerr << "Opening existing service failed, error: " << err << std::endl;
			}
		}
      if(installed == true) 
         std::cout << "Service " << SVCNAME << " reinstalled successfully. You can start service from control panel or with command: \"net start " << SVCNAME << "\"" << std::endl;
      
	} else {
      SERVICE_DESCRIPTION sd;
      sd.lpDescription = SVCDESCRIPTION;
      ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, &sd);
		std::cout << "Service " << SVCNAME << " installed successfully. You can start service from control panel or with command: \"net start " << SVCNAME << "\"" << std::endl;
      installed = true;
		CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);

	if (installed) {
		char serviceRegPath[MAX_PATH];
		lstrcpy(serviceRegPath, "System\\CurrentControlSet\\Services\\");
		lstrcat(serviceRegPath, SVCNAME);

		DWORD disp = 0;
		HKEY hKey = NULL;
		DWORD err = RegCreateKeyExA(HKEY_LOCAL_MACHINE, serviceRegPath, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);
		if (err == 0) {
			std::string new_cmd_line = szPath;
			RegSetValueEx(hKey, "ImagePath", 0, REG_SZ, (BYTE*)new_cmd_line.c_str(), (DWORD)new_cmd_line.size() + 1);
			RegCloseKey(hKey);
		}
		else {
         installed = false;// cannot change command line
         std::cerr << "Cannot setup command line, error: " << err << std::endl;
		}
	}
   if(!installed)
      err = GetLastError();
	return err;
}

DWORD remove_win_service()
{
	SC_HANDLE   schService;
	SC_HANDLE   schSCManager;
	SERVICE_STATUS          ssStatus;
	char                   szErr[256];
	bool deleted = false;
	DWORD err = 0;

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
            std::cout << "Stopping " << SVCNAME << "." << std::endl;
				Sleep(1000);
				DWORD counter = 0;
				while (QueryServiceStatus(schService, &ssStatus) || counter < 60) {

					if (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
                  std::cout << ".";
						Sleep(1000);
					}
					else
						break;
					counter++;
				}

				if (ssStatus.dwCurrentState == SERVICE_STOPPED)					
               std::cout << SVCNAME << " stopped." << std::endl;
				else
               std::cerr << SVCNAME << " failed to stop." << std::endl;
			}
			// remove the service
			if (DeleteService(schService)) {
            std::cout << SVCNAME << " removed." << std::endl;
				deleted = true;
			} 
         else {
            err = GetLastError();
            std::cerr << "DeleteService failed, error: " << GetLastErrorText(szErr, 256) << std::endl;
         }

			CloseServiceHandle(schService);
      } else {
         err = GetLastError();
         std::cerr << "OpenService failed, error: " << GetLastErrorText(szErr, 256) << std::endl;
      }

		CloseServiceHandle(schSCManager);
   } else {
      err = GetLastError();
      std::cerr << "OpenSCManager failed, error: " << GetLastErrorText(szErr, 256) << std::endl;
   }

	return err;
}

DWORD InitializeService()
{
	s_SvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		SvcCtrlHandler);

	if (!s_SvcStatusHandle) {
		SvcReportEvent((char*)"RegisterServiceCtrlHandler");
      return GetLastError();
	}

	// These SERVICE_STATUS members remain as set here
	s_SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	s_SvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM
	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
   
	return 0;
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

		StopWinService();
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
		StringCchPrintf(Buffer, 80, "%s failed with %d", szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
         0,                   // event identifier
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
