#include <atomic>
#include "TrainerLib.h"
#include "MinHookLib.h"
#include "ThreadProxy.h"
#include <Psapi.h>

namespace TrainerLib
{
	std::atomic<bool> _running;
	std::unique_ptr<TrainerLib> _instance = nullptr;

	DWORD WINAPI Run(init_args* args)
	{
		if (_running.exchange(true)) {
			return 1;
		}

		_instance = std::make_unique<TrainerLib>(args);

		_instance->Execute();

		MH_Uninitialize();
		MH_Initialize();

		_instance = nullptr;

		_running.store(false);

		return 0;
	}

	bool ThreadRedirectionEnabled()
	{
		wchar_t moduleName[MAX_PATH];
		return GetModuleBaseNameW(GetCurrentProcess(), nullptr, moduleName, MAX_PATH - 1) 
			&& std::wstring(moduleName) == L"blackops3.exe";
	}

	extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
	{
		if (fdwReason == DLL_PROCESS_ATTACH) {
			MH_Initialize();
			if (ThreadRedirectionEnabled()) {
				InitializeThreadProxyer();
			}
		} else if (fdwReason == DLL_PROCESS_DETACH) {
			if (ThreadRedirectionEnabled()) {
				UninitializeThreadProxyer();
			}
			MH_Uninitialize();
		}

		if (_instance != nullptr) {
			return _instance->HandleDllMain(hModule, fdwReason);
		}

		return TRUE;
	}
}
