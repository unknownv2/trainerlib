#include <Windows.h>
#include <string>
#include "Injector.h"
#include "Executor.h"
#include "Errors.h"
#include "Util.h"

#define PROC_ACCESS PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE

HANDLE OpenProc(DWORD dwProcessId)
{
	auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

	if (hProcess == nullptr) {
		hProcess = OpenProcess(PROC_ACCESS, FALSE, dwProcessId);
	}

	return hProcess;
}

DWORD wmain(int argc, const wchar_t** argv)
{
	if (argc < 10) {
		return ERROR_INVALID_ARGUMENTS;
	}

	// Get the path to TrainerLib (this .exe renamed to .bin)
	wchar_t path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, MAX_PATH);

	auto trainerLibPath = std::wstring(path);
	trainerLibPath[trainerLibPath.length() - 3] = 'b';
	trainerLibPath[trainerLibPath.length() - 2] = 'i';
	trainerLibPath[trainerLibPath.length() - 1] = 'n';

	// Read all the command-line arguments in.
	auto processId = stoul(std::wstring(argv[1]));
	auto logPipe = std::wstring(argv[2]);
	auto messengerPipe = std::wstring(argv[3]);
	auto trainerDll = std::wstring(argv[4]);
	auto flags = WStringToUTF8(std::wstring(argv[5]));
	auto timeout = uint32_t(stoul(std::wstring(argv[6])));
	auto chainedCommand = std::wstring(argv[7]);
	auto chainedArgs = std::wstring(argv[8]);
	auto chainedCwd = std::wstring(argv[9]);
	auto secondPass = argc > 10 && *argv[argc - 1] == '1';

	auto hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

	if (hProcess == nullptr) {
		PrintError("The process doesn't exist.");
		return ERROR_PROC_EXITED;
	}

	DWORD dwExitCode;
	if (!GetExitCodeProcess(hProcess, &dwExitCode)) {
		PrintError("Failed to get exit code of remote process.");
		return ERROR_PROC_EXITED;
	}

	if (dwExitCode != STILL_ACTIVE) {
		PrintError("Remote process has exited.");
		return ERROR_PROC_EXITED;
	}

	CloseHandle(hProcess);

	// Open the game's process.
	hProcess = OpenProc(processId);

	if (hProcess == nullptr) {
		if (GrantSeDebugPrivilege()) {
			hProcess = OpenProc(processId);
		}

		if (hProcess == nullptr) {
			if (!secondPass) {
				// Attempt to elevate.
				return RunElevated(argc, argv);
			}

			hProcess = OpenProc(processId);

			// We know we're elevated at this point.
			if (hProcess == nullptr) {
				std::cout << "Failed to open process " << processId << "." << std::endl;
				PrintError();
				return ERROR_PROC_OPEN;
			}
		}
	}

	DWORD error;
	
	// Inject TrainerLib into the game.
	HMODULE hModule;
	if ((error = Inject(hProcess, trainerLibPath, timeout, hModule)) != ERROR_OK) {
		CloseHandle(hProcess);
		return error;
	}

	// Set up the arguments passed to TrainerLib.
	init_args args = {};
	logPipe._Copy_s(args.logPipe, sizeof init_args::logPipe, logPipe.size());
	messengerPipe._Copy_s(args.infinityPipe, sizeof init_args::infinityPipe, messengerPipe.size());
	trainerDll._Copy_s(args.trainerDllPath, sizeof init_args::trainerDllPath, trainerDll.size());
	flags._Copy_s(args.trainerFlags, sizeof init_args::trainerFlags, flags.size());

	// Execute TrainerLib.
	if ((error = Execute(hProcess, hModule, args)) != ERROR_OK) {
		CloseHandle(hProcess);
		return error;
	}

	if (chainedCommand.length() != 0) {
		if (!ExecuteChained(chainedCommand, chainedArgs, chainedCwd)) {
			PrintError("Failed to execute chained command.");
			return ERROR_EXEC_CHAINED_COMMAND;
		}
	}

	return ERROR_OK;
}