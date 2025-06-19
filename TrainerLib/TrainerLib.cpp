#include <memory>
#include <AclAPI.h>
#include "TrainerLib.h"
#include "ThreadProxy.h"
#include "Process.h"
#include "TrainerArgs.h"
#include "ILogger.h"
#include "MonoRuntime.h"
#include "IHooker_001.h"
#include "Debugger.h"
#include "IAssembler.h"
#include "IRemoteClient.h"
#include "TaskManager.h"
#include "NamedPipeLogger.h"
#include "NamedPipeMessenger.h"
#include "Infinity.h"
#include "UnhandledExceptionCatcher.h"
#include "MinHooker.h"
#include "BindingAssembler.h"
#include "StdOutLogger.h"
#include "Config.h"
#include "Speedhack.h"
#include "DebugOutputRedirector.h"

#ifdef TRAINERLIB_USING_BASM
#include "BasmAssembler.h"
#else
#include "CETackAssembler.h"
#endif

namespace TrainerLib
{
	bool TrainerLib::ExecuteInternal()
	{
		_process = std::make_shared<Process>();
		_logger->Log(LogLevel::Debug, L"Game Timestamp: %d", _process->GetModuleTimestamp());

		auto gameVersion = _args->gameVersion;

		if (gameVersion == 0) {
			gameVersion = _process->GetModuleTimestamp();
		}

		_meta = std::make_shared<TrainerArgs>(_args->trainerFlags, gameVersion);

		if (_meta->HasFlags()) {
			_logger->Log(LogLevel::Info, "Flags: %s", _meta->GetFlags());
		}

		_logger->Log(LogLevel::Debug, L"Trainer DLL: %s", _args->trainerDllPath);

		auto messenger = std::make_shared<NamedPipeMessenger>(_logger, _args->infinityPipe);

		_hooker = std::make_shared<MinHooker>(_logger);

		auto debugDirector = _meta->HasFlag("debug")
			? std::make_unique<DebugOutputRedirector>(_logger, _hooker)
			: nullptr;

		_client = std::make_shared<Infinity>(
			messenger, 
			_logger, 
			move(std::make_shared<UnhandledExceptionCatcher>(_logger, _hooker)));

		if (!_client->Connect()) {
			return false;
		}

		_taskManager = std::make_shared<TaskManager>(_logger);

#ifdef TRAINERLIB_USING_BASM
		auto basm = std::shared_ptr<basm::IBasm>(basm::MakeInstance());
		basm->SetLogger(_logger.get());
		auto baseAssembler = std::make_unique<BasmAssembler>(move(basm), _logger);
#else
		auto baseAssembler = std::make_unique<CETackAssembler>(_logger);
#endif
		
		_logger->Log(LogLevel::Debug, "Loading the trainer into the process...");

		_hTrainer = LoadLibraryW(_args->trainerDllPath);

		if (_hTrainer == nullptr) {
			_logger->Log(LogLevel::Error, "Failed to load trainer DLL!");
			return false;
		}

		_client->AddValueChangedHandler(this);

		_logger->Log(LogLevel::Debug, "Initializing assembler...");

		_assembler = std::make_shared<BindingAssembler>(_client, _logger, move(baseAssembler));

		auto allowedVars = _args->initialVars;
		for (uint32_t x = 0; x < _args->initialVarCount; x++) {
			auto str = std::string(allowedVars, 32);
			_assembler->PredefineSymbol(str.c_str());
			allowedVars += 32;
		}

		_logger->Log(LogLevel::Info, "Calling trainer Setup()...");

		auto setupFunction = reinterpret_cast<bool(*)(ITrainerLib*)>(GetProcAddress(_hTrainer, "Initialize"));

		if (setupFunction == nullptr) {
			_logger->Log(LogLevel::Error, "Trainer setup export does not exist!");
			return false;
		}

		if (!messenger->NotifyInitializationStarted()) {
			_logger->Log(LogLevel::Error, "Failed to notify client.");
			return false;
		}

		if (!setupFunction(this)) {
			_logger->Log(LogLevel::Error, "Trainer setup failed.");
			return false;
		}

		_logger->Log(LogLevel::Info, "Trainer setup complete! Initialization is finished.");

		if (!messenger->NotifyInitializationIsComplete()) {
			_logger->Log(LogLevel::Error, "Failed to notify client.");
			return false;
		}

		_client->RunLoop();

		_logger->Log(LogLevel::Info, "Client disconnected.");

		_taskManager->EndAllTasks();

		auto cleanupFunction = reinterpret_cast<bool(*)()>(GetProcAddress(_hTrainer, "Clean"));

		if (cleanupFunction == nullptr) {
			_logger->Log(LogLevel::Debug, "No Cleanup() function found in trainer. Skipping...");
		} else {
			_logger->Log(LogLevel::Info, "Calling Cleanup() function...");

			if (!cleanupFunction()) {
				_logger->Log(LogLevel::Error, "Trainer cleanup failed.");
				return false;
			}
		}

		return true;
	}

	DWORD WINAPI TrainerLib::ExecuteThread(LPVOID arg)
	{
		auto trainerLib = static_cast<TrainerLib*>(arg);

		bool success;

		try {
			success = trainerLib->ExecuteInternal();
		}
		catch (...) {
			success = false;
		}

		if (!success) {
			trainerLib->_logger->Log(LogLevel::Error, "An error occured. Last Win32: 0x%08x", GetLastError());
		}

		return success ? 0 : 1;
	}

	void TrainerLib::Execute()
	{
		// Open the logger right away.
		if (_args->logPipe[0] == 0) {
			_logger = std::make_shared<StdOutLogger>();
		} else {
			_logger = std::make_shared<NamedPipeLogger>(_args->logPipe);
		}

		_logger->Log(LogLevel::Info, "We're in! PID: %d, TID: %d", GetCurrentProcessId(), GetCurrentThreadId());

		HANDLE waitHandles[2];

		// Execute the trainer in a new thread. We do this so we can wait on it.
		DWORD threadId;
		waitHandles[0] = CreateProxyThread(nullptr, 0, ExecuteThread, this, 0, &threadId);

		// If the trainer thread couldn't be created, we're screwed.
		if (waitHandles[0] == nullptr) {
			_logger->Log(LogLevel::Error, "Failed to create TrainerLib thread. Last Win32: 0x%08x", GetLastError());
			return;
		}

		// Create an event that can be signaled to unload the trainer.
		waitHandles[1] = CreateUnloadEvent();

		// Don't wait on the event handle if we failed to create it.
		DWORD waitHandleCount = waitHandles[1] == nullptr ? 1 : 2;

		// Wait for TrainerLib to exit or for the unload event to be signaled.
		auto result = WaitForMultipleObjects(waitHandleCount, waitHandles, FALSE, INFINITE);

		if (result == WAIT_FAILED) {
			_logger->Log(LogLevel::Error, "Failed to wait on objects. Last Win32: 0x%08x", GetLastError());

			// Kill the trainer thread.
			TerminateThread(waitHandles[0], 1);
			CloseHandle(waitHandles[0]);

			// Close the unload event handle.
			if (waitHandleCount == 2) {
				CloseHandle(waitHandles[1]);
			}

			return;
		}

		// If we weren't signaled, we can close everything.
		if (result - WAIT_OBJECT_0 != 1) {

			// Close the trainer thread handle.
			CloseHandle(waitHandles[0]);

			// Close the unload event handle.
			if (waitHandleCount == 2) {
				CloseHandle(waitHandles[1]);
			}

			return;
		}

		_logger->Log(LogLevel::Info, "Signaled to unload. Waiting for trainer...");

		// We were told to unload the trainer from the game. We can force 
		// TrainerLib to stop executing by closing its connection to the client.

		// Cancel any pending I/O operations. Kind of a leak of implementation logic...
		CancelSynchronousIo(waitHandles[0]);

		_client->Disconnect();

		// Gracefully wait till the thread exits.
		result = WaitForSingleObject(waitHandles[0], 5000);

		// Or kill it if it takes too long (5 seconds).
		if (result == WAIT_FAILED) {
			_logger->Log(LogLevel::Warning, "Failed to wait on trainer thread. Killing it. Last Win32: 0x%08x", GetLastError());
			TerminateThread(waitHandles[0], 1);
		}

		// Cleanup handles.
		CloseHandle(waitHandles[0]);
		CloseHandle(waitHandles[1]);
	}

	HANDLE TrainerLib::CreateUnloadEvent() const
	{
		EXPLICIT_ACCESS ea{};
		PSID pEveryoneSID = nullptr;

		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;

		if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID)) {
			_logger->Log(LogLevel::Warning, "AllocateAndInitializeSid failed. Last Win32: %08x", GetLastError());
			return nullptr;
		}

		ea.grfAccessPermissions = SPECIFIC_RIGHTS_ALL | STANDARD_RIGHTS_ALL;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance = NO_INHERITANCE;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName = static_cast<LPWSTR>(pEveryoneSID);

		PACL pACL;

		if (SetEntriesInAclW(1, &ea, nullptr, &pACL) != ERROR_SUCCESS) {
			_logger->Log(LogLevel::Warning, "SetEntriesInAcl failed. Last Win32: %08x", GetLastError());
			return nullptr;
		}

		auto sd = static_cast<PSECURITY_DESCRIPTOR>(LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));

		if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION)) {
			_logger->Log(LogLevel::Warning, "InitializeSecurityDescriptor failed. Last Win32: %08x", GetLastError());
			return nullptr;
		}

		if (!SetSecurityDescriptorDacl(sd, TRUE, pACL, FALSE)) {
			_logger->Log(LogLevel::Warning, "SetSecurityDescriptorDacl failed. Last Win32: %08x", GetLastError());
			return nullptr;
		}

		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = sd;
		sa.bInheritHandle = false;

		auto hUnloadEvent = CreateEventW(&sa, TRUE, FALSE, _unloadSignalName);

		if (hUnloadEvent == nullptr) {
			_logger->Log(LogLevel::Warning, "Failed to create unload event. Last Win32: %08x", GetLastError());
			return nullptr;
		}

		return hUnloadEvent;
	}

	void TrainerLib::InitDebugger()
	{
		_varLock.lock();
		if (_debugger == nullptr) {
			_debugger = std::make_unique<Debugger>(_logger);
		}
		_varLock.unlock();
	}

	void TrainerLib::InitMono()
	{
		_varLock.lock();
		if (_mono == nullptr) {
			_mono = std::make_shared<MonoRuntime>(_logger);
		}
		_varLock.unlock();
	}

	bool TrainerLib::FreeTrainerDll()
	{
		auto hTrainer = _hTrainer;

		if (hTrainer != nullptr && FreeLibrary(hTrainer) == FALSE) {
			return false;
		}

		_hTrainer = nullptr;

		return true;
	}

	BOOL TrainerLib::HandleDllMain(HMODULE, DWORD fdwReason) const
	{
		switch (fdwReason)
		{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			if (_debugger != nullptr) {
				_debugger->RegisterThread(GetCurrentThread());
			}
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
		default:
			break;
		}

		return TRUE;
	}

	void TrainerLib::HandleValueChanged(const char* name, double value)
	{
		std::string nameString = name;

		if (nameString != "game_speed") {
			return;
		}

		if (_speedhack == nullptr) {
			_speedhack = std::make_unique<Speedhack>(_hooker, _logger);
		}

		_speedhack->SetSpeed(value);
	}

	void* TrainerLib::GetInterface(const char* pszName)
	{
		std::string name = pszName;

		if (name == ITrainerArgs_001_Version) {
			return static_cast<ITrainerArgs_001*>(_meta.get());
		}

		if (name == IProcess_001_Version) {
			return static_cast<IProcess_001*>(_process.get());
		}

		if (name == ILogger_001_Version) {
			return static_cast<ILogger_001*>(_logger.get());
		}

		if (name == IHooker_001_Version) {
			return static_cast<IHooker_001*>(_hooker.get());
		}

		if (name == IAssembler_001_Version) {
			return static_cast<IAssembler_001*>(_assembler.get());
		}

		if (name == IRemoteClient_001_Version) {
			return static_cast<IRemoteClient_001*>(_client.get());
		}

		if (name == ITaskManager_001_Version) {
			return static_cast<ITaskManager_001*>(_taskManager.get());
		}

		if (name == IDebugger_001_Version) {
			InitDebugger();
			return static_cast<IDebugger_001*>(_debugger.get());
		}

		if (name == IMonoRuntime_001_Version) {
			InitMono();
			return static_cast<IMonoRuntime_001*>(_mono.get());
		}

		if (_logger != nullptr) {
			_logger->Log(LogLevel::Warning, "Unknown interface \"%s\" requested.", name.c_str());
		}

		return nullptr;
	}
}
