#pragma once

#include <Windows.h>
#include <memory>
#include <vector>
#include <codecvt>
#include <iostream>
#include <iomanip>

std::unique_ptr<std::vector<HMODULE>> GetAllModuleHandles(HANDLE hProcess);

HMODULE GetModuleHandleByFileName(HANDLE hProcess, std::wstring fileName, const std::vector<HMODULE>& handles);

bool IsElevated(HANDLE hProcess);

bool RequiresElevation(HANDLE hProcess);

DWORD RunElevated(int argc, const wchar_t** argv);

bool GrantSeDebugPrivilege();

bool ExecuteChained(std::wstring command, std::wstring args, std::wstring cwd);

inline std::string WStringToUTF8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

void PrintError(const char* message = nullptr);

inline void PrintLine(const char* message)
{
	std::cout << message << std::endl;

	std::cout.flush();
}
