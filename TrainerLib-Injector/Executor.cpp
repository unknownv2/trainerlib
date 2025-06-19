#include "Executor.h"
#include "Errors.h"
#include "Util.h"

int Execute(HANDLE hProcess, HMODULE hModule, const init_args& args)
{
	// Read the export address that was patched
	// into the DOS stub of TrainerLib's module.
	LPTHREAD_START_ROUTINE lpExportAddress;

	SIZE_T bytesRead;
	if (!ReadProcessMemory(
		hProcess,
		reinterpret_cast<void*>(uintptr_t(hModule) + sizeof IMAGE_DOS_HEADER),
		&lpExportAddress, 
		sizeof LPTHREAD_START_ROUTINE, 
		&bytesRead) || lpExportAddress == nullptr) {
		PrintError("Failed to read export address from remote process.");
		return ERROR_NO_EXPORT;
	}

	// Allocate and write the arguments into the game's process.
	auto bufferLen = sizeof(init_args);
	auto lpBuffer = VirtualAllocEx(hProcess, nullptr, bufferLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (lpBuffer == nullptr) {
		PrintError("Failed to allocate memory for trainer args in remote process.");
		return ERROR_REMOTE_ALLOC;
	}

	SIZE_T bytesWritten;
	if (!WriteProcessMemory(hProcess, lpBuffer, &args, bufferLen, &bytesWritten) || bytesWritten != bufferLen) {
		PrintError("Failed to write trainer args to remote process.");
		VirtualFreeEx(hProcess, lpBuffer, 0, MEM_RELEASE);
		return ERROR_REMOTE_WRITE;
	}

	// Execute TrainerLib in the game.
	auto hThread = CreateRemoteThreadEx(hProcess, nullptr, 0, lpExportAddress, lpBuffer, 0, nullptr, nullptr);

	if (hThread == nullptr) {
		PrintError("CreateRemoteThread failed on TrainerLib.");
		VirtualFreeEx(hProcess, lpBuffer, 0, MEM_RELEASE);
		return ERROR_REMOTE_EXEC;
	}

	return ERROR_OK;
}
