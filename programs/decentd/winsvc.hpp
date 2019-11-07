#pragma once
#include <windows.h>

#define SVCNAME "DCore"
#define SVCDISPLAYNAME "DCore Network Node"
#define SVCDESCRIPTION "Fast, powerful and cost-efficient blockchain"

DWORD install_win_service();
DWORD remove_win_service();
