#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include "Util.h"
#include "Errors.h"

std::unique_ptr<std::vector<HMODULE>> GetAllModuleHandles(HANDLE hProcess)
{
	auto handles = std::make_unique<std::vector<HMODULE>>(64);

	DWORD size = 0;

	for (;;)
	{
		int x;
		auto success = false;

		// Try 50 times because it can fail during normal operation.
		for (x = 0; x < 50 && !success; x++)
		{
			success = EnumProcessModulesEx(
				hProcess,
				handles->data(),
				sizeof(HMODULE) * static_cast<DWORD>(handles->capacity()),
				&size,
				LIST_MODULES_ALL) != FALSE;
		}

		if (x == 50) {
			return nullptr;
		}

		size /= sizeof(HMODULE);

		if (size <= handles->capacity()) {
			break;
		}

		handles->resize(handles->capacity() * 2);
	}

	return move(handles);
}

HMODULE GetModuleHandleByFileName(HANDLE hProcess, std::wstring fileName, const std::vector<HMODULE>& handles)
{
	transform(fileName.begin(), fileName.end(), fileName.begin(), tolower);

	wchar_t name[MAX_PATH];

	for (auto hModule : handles)
	{
		if (GetModuleFileNameExW(hProcess, hModule, name, MAX_PATH) == fileName.length())
		{
			name[fileName.length()] = 0;

			std::wstring moduleName = name;
			transform(moduleName.begin(), moduleName.end(), moduleName.begin(), tolower);

			if (moduleName == fileName) {
				return hModule;
			}
		}
	}

	return nullptr;
}

bool IsElevated(HANDLE hProcess)
{
	HANDLE hToken;

	if (!OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
		return false;
	}

	TOKEN_ELEVATION elevation;
	DWORD cbSize;

	auto result = GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize);

	CloseHandle(hToken);

	return result && elevation.TokenIsElevated;
}

bool RequiresElevation(HANDLE hProcess)
{
	return !IsElevated(GetCurrentProcess()) && IsElevated(hProcess);
}

DWORD RunElevated(int argc, const wchar_t** argv)
{
	PrintLine("Attempting to elevate process...");

	std::wstring args = L"";

	for (auto x = 1; x < argc; x++) {
		args += L"\"" + std::wstring(argv[x]) + L"\" ";
	}

	args += L" 1";

	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
	shExInfo.hwnd = nullptr;
	shExInfo.lpVerb = L"runas";
	shExInfo.lpFile = argv[0];
	shExInfo.lpParameters = args.c_str();
	shExInfo.lpDirectory = nullptr;
	shExInfo.nShow = SW_HIDE;
	shExInfo.hInstApp = nullptr;

	if (!ShellExecuteEx(&shExInfo)) {
		PrintLine("ShellExecuteEx failed.");
		return ERROR_FAILED_ELEVATION;
	}

	if (shExInfo.hProcess == nullptr) {
		PrintError("ShellExecute process handle is null.");
		return ERROR_FAILED_ELEVATION;
	}

	WaitForSingleObject(shExInfo.hProcess, INFINITE);

	DWORD dwExitCode;
	if (!GetExitCodeProcess(shExInfo.hProcess, &dwExitCode)) {
		PrintLine("Failed to get exit code of elevated process.");
		CloseHandle(shExInfo.hProcess);
		return ERROR_FAILED_ELEVATION;
	}

	CloseHandle(shExInfo.hProcess);

	return dwExitCode;
}

bool GrantSeDebugPrivilege()
{
	PrintLine("Attemping to grant SeDebugPrivilege...");

	HANDLE hToken;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
		if (GetLastError() != ERROR_NO_TOKEN) {
			PrintError("OpenThreadToken failed.");
			return false;
		}

		if (!ImpersonateSelf(SecurityImpersonation)) {
			PrintError("ImpersonateSelf failed.");
			return false;
		}

		if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
			PrintError("OpenThreadToken failed twice.");
			return false;
		}
	}

	LUID luid;
	TOKEN_PRIVILEGES tokenPrivileges;

	if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid)) {
		PrintError("LookupPrivilegeValue failed.");
		CloseHandle(hToken);
		return false;
	}

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = luid;
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(
		hToken,
		FALSE,
		&tokenPrivileges,
		sizeof(TOKEN_PRIVILEGES),
		nullptr,
		nullptr);

	auto result = GetLastError() == ERROR_SUCCESS;

	if (!result) {
		PrintError("AdjustTokenPrivileges failed.");
	}

	CloseHandle(hToken);

	return result;
}

bool ExecuteChained(std::wstring command, std::wstring args, std::wstring cwd)
{
	STARTUPINFOW startInfo {};
	PROCESS_INFORMATION procInfo;

	args = command + L" " + args;

	return CreateProcessW(
		command.c_str(),
		const_cast<wchar_t*>(args.c_str()),
		nullptr,
		nullptr,
		false,
		CREATE_NO_WINDOW,
		nullptr,
		cwd.length() != 0 ? cwd.c_str() : nullptr,
		&startInfo,
		&procInfo) != FALSE;
}

void PrintError(const char* message)
{
	if (message != nullptr) {
		std::cout << message << std::endl;
	}

	char code[9] {};
	sprintf_s(code, 9, "%08x", GetLastError());

	std::cout << "Last Win32 error: 0x" << code << std::endl;

	std::cout.flush();
}
