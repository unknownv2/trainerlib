#include "Injector.h"
#include "Errors.h"
#include "Util.h"

int Inject(HANDLE hProcess, std::wstring dllPath, uint32_t& timeout, HMODULE& hModule)
{
	auto loadLibraryProc = LPTHREAD_START_ROUTINE(LoadLibraryW);

	auto pathLength = (dllPath.length() + 1) * sizeof(wchar_t);
	auto lpImagePath = VirtualAllocEx(hProcess, nullptr, pathLength, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (lpImagePath == nullptr) {
		std::wcout << L"Failed to allocate memory for TrainerLib path (" << dllPath.c_str() << L") in remote process." << std::endl;
		PrintError();
		return ERROR_REMOTE_WRITE;
	}

	SIZE_T bytesWritten;
	if (!WriteProcessMemory(hProcess, lpImagePath, dllPath.c_str(), pathLength, &bytesWritten) || bytesWritten != pathLength) {
		PrintError("Failed to write TrainerLib path to remote process.");
		VirtualFreeEx(hProcess, lpImagePath, 0, MEM_RELEASE);
		return ERROR_REMOTE_WRITE;
	}

	auto hThread = CreateRemoteThreadEx(hProcess, nullptr, 0, loadLibraryProc, lpImagePath, 0, nullptr, nullptr);

	if (hThread == nullptr) {
		PrintError("CreateRemoteThreadEx failed on LoadLibrary in remote process.");
		VirtualFreeEx(hProcess, lpImagePath, 0, MEM_RELEASE);
		return ERROR_REMOTE_EXEC;
	}

	WaitForSingleObject(hThread, INFINITE);

	DWORD dwExitCode;
	if (!GetExitCodeThread(hThread, &dwExitCode)) {
		PrintError("Failed to get LoadLibrary thread exit code.");
		CloseHandle(hThread);
		return ERROR_LOAD_LIBRARY;
	}

	// We have to enumerate the modules on x64 because the
	// exit code return value was truncated.
#ifdef _AMD64_
	auto handles = GetAllModuleHandles(hProcess);

	if (handles == nullptr) {
		PrintError("Failed to get module handles.");
		return ERROR_ENUMERATE_MODULES;
	}

	hModule = GetModuleHandleByFileName(hProcess, dllPath, *handles);

	if (hModule == nullptr) {
		PrintError("TrainerLib module not found in remote process.");
		return ERROR_MODULE_NOT_FOUND;
	}
#else
	if (dwExitCode == 0) {
		PrintError("LoadLibrary failed.");
		return ERROR_LOAD_LIBRARY;
	}

	hModule = HMODULE(dwExitCode);
#endif

	return ERROR_OK;
}
