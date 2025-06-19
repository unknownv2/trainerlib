#pragma once

#include <Windows.h>
#include "TrainerArgs.h"
#include "IRemoteClient.h"
#include "IProcess.h"
#include "ILogger.h"
#include "TaskManager.h"
#include "Debugger.h"
#include "IHooker.h"
#include "MonoRuntime.h"
#include "IAssembler.h"
#include "Speedhack.h"

namespace TrainerLib
{
	#pragma pack(push) 
	#pragma pack(1)
	struct init_args
	{
		wchar_t logPipe[256];
		wchar_t infinityPipe[256];
		wchar_t trainerDllPath[256];
		char trainerFlags[256];
		uint32_t gameVersion;
		uint32_t initialVarCount;
		char initialVars[32];
	};

	void* GetInterface(const char* name);

	// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
	class ITrainerLib
	{
	public:
		virtual void* GetInterface(const char* name) = 0;
	};

	// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
	class TrainerLib : public ITrainerLib, public IValueChangedHandler
	{
		const wchar_t* _unloadSignalName = L"Local\\Infinity_Trainer";

		std::mutex _varLock;
		init_args* _args;
		HMODULE _hTrainer = nullptr;

		std::shared_ptr<TrainerArgs> _meta;
		std::shared_ptr<IRemoteClient> _client;
		std::shared_ptr<IProcess> _process;
		std::shared_ptr<ILogger> _logger;
		std::shared_ptr<TaskManager> _taskManager;
		std::unique_ptr<Debugger> _debugger = nullptr;
		std::shared_ptr<IAssembler> _assembler;
		std::shared_ptr<IHooker> _hooker;
		std::shared_ptr<MonoRuntime> _mono;
		std::unique_ptr<Speedhack> _speedhack;

		HANDLE CreateUnloadEvent() const;
		bool ExecuteInternal();
		void InitDebugger();
		void InitMono();
		bool FreeTrainerDll();

		static DWORD WINAPI ExecuteThread(LPVOID arg);

	public:
		explicit TrainerLib(init_args* args)
			: _args(args)
		{
			
		}

		void Execute();

		void* GetInterface(const char* pszName) override;

		BOOL HandleDllMain(HMODULE hModule, DWORD fdwReason) const;

		void HandleValueChanged(const char* name, double value) override;

		~TrainerLib()
		{
			FreeTrainerDll();
			_debugger = nullptr;
		}
	};
}
