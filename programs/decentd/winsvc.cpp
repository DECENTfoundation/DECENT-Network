#include "winsvc.hpp"

#include <iostream>
#include <vector>

LPSTR GetLastErrorText(LPSTR lpszBuf, DWORD dwSize);

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
