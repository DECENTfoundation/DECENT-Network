#pragma once

int install_win_service(const char *cmd_line_str);
bool IsRunningAsSystemService();
void GetAppDataDir(char* path, int max_len);
void StopWinService();


