#pragma once
#include <string>

#define SVCNAME "DCore"
#define SVCDISPLAYNAME "DCore Network Node"
#define SVCDESCRIPTION "Fast, powerful and cost-efficient blockchain"

DWORD install_win_service();
DWORD remove_win_service();
std::string GetAppDataDir();
bool IsRunningAsSystemService();
void StopWinService();

DWORD InitializeService();
void ReportSvcStatus(DWORD, DWORD, DWORD);
void SvcReportEvent(LPTSTR);
