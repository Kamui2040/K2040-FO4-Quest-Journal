#pragma once
// TEST-ONLY shim for Linux-hosted JournalSettings.cpp tests. Not used in game builds.
#include <filesystem>
#include <system_error>
#include <string>
#include <cwchar>
using DWORD = unsigned long;
inline std::wstring kTestLocalAppData;
inline DWORD GetEnvironmentVariableW(const wchar_t* key, wchar_t* output, DWORD capacity) {
    if(std::wcscmp(key,L"LOCALAPPDATA") || kTestLocalAppData.empty())return 0;
    if(capacity <= kTestLocalAppData.size())return static_cast<DWORD>(kTestLocalAppData.size()+1);
    std::wcscpy(output,kTestLocalAppData.c_str());
    return static_cast<DWORD>(kTestLocalAppData.size());
}
#define MOVEFILE_REPLACE_EXISTING 0x1
#define MOVEFILE_WRITE_THROUGH 0x8
inline int MoveFileExW(const wchar_t* source, const wchar_t* destination, unsigned long) {
    std::error_code error;
    std::filesystem::rename(std::filesystem::path(source), std::filesystem::path(destination), error);
    return error ? 0 : 1;
}
