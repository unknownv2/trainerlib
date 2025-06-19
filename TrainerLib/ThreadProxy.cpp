#include "ThreadProxy.h"
#include "MinHookLib.h"

#define THREAD_PROXY_MAGIC 0x20202059584F5250 // "PROXY   "

namespace TrainerLib
{
	void* _exitProcessTarget = nullptr;
	ExitProcessT _exitProcessOriginal = nullptr;

	HANDLE WINAPI CreateProxyThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, __drv_aliasesMem LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
	{
		if (!_exitProcessTarget) {
			return CreateThread(
				lpThreadAttributes,
				dwStackSize,
				lpStartAddress,
				lpParameter,
				dwCreationFlags,
				lpThreadId);
		}

		auto proxyArgs = new ThreadStartArgs();
		proxyArgs->Magic = THREAD_PROXY_MAGIC;
		proxyArgs->EntryPoint = lpStartAddress;
		proxyArgs->Parameter = lpParameter;

		return CreateThread(
			lpThreadAttributes,
			dwStackSize,
			LPTHREAD_START_ROUTINE(_exitProcessTarget),
			proxyArgs,
			dwCreationFlags,
			lpThreadId);
	}

	DWORD WINAPI ExitProcessDetour(ThreadStartArgs* exitCodeOrThreadArgs)
	{
		ThreadStartArgs args = { 0 };
		try {
			args = *static_cast<ThreadStartArgs*>(exitCodeOrThreadArgs);
		} catch (...) {
			// Making sure memory can be accessed.
		}

		if (args.Magic == THREAD_PROXY_MAGIC) {
			VirtualFree(exitCodeOrThreadArgs, 0, MEM_RELEASE);
			return args.EntryPoint(args.Parameter);
		} else {
			_exitProcessOriginal(DWORD(exitCodeOrThreadArgs));
			return 0;
		}
	}

	void InitializeThreadProxyer()
	{
		_exitProcessTarget = GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess");
		MH_Initialize();
		MH_CreateHook(_exitProcessTarget, ExitProcessDetour, reinterpret_cast<void**>(&_exitProcessOriginal));
		MH_EnableHook(_exitProcessTarget);
	}

	void UninitializeThreadProxyer()
	{
		if (_exitProcessTarget) {
			MH_DisableHook(_exitProcessTarget);
			MH_RemoveHook(_exitProcessTarget);
			_exitProcessTarget = nullptr;
			_exitProcessOriginal = nullptr;
		}
	}
}