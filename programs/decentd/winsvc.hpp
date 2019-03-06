#pragma once


#define SVCNAME "decentd"
#define SVCDISPLAYNAME "DECENT Network Node"
#define SVCDESCRIPTION "Synchronizes with other nodes on DECENT Network and provides services."

DWORD install_win_service(const char *cmd_line_str);
DWORD remove_win_service();
bool IsRunningAsSystemService();
void GetAppDataDir(char* path, int max_len);
void StopWinService();

DWORD InitializeService();
void ReportSvcStatus(DWORD, DWORD, DWORD);
void SvcReportEvent(LPTSTR);


