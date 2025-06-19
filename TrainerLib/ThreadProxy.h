#pragma once

#include <Windows.h>

namespace TrainerLib
{
	typedef VOID(WINAPI *ExitProcessT)(DWORD dwExitCode);

#ifdef _AMD64_
	struct ThreadStartArgs {
		ULONGLONG Magic;
		PTHREAD_START_ROUTINE EntryPoint;
		LPVOID Parameter;
	};
#else
	struct ThreadStartArgs {
		ULONGLONG Magic;
		PTHREAD_START_ROUTINE EntryPoint;
		DWORD Padding1;
		LPVOID Parameter;
		DWORD Padding2;
	};
#endif

	_Ret_maybenull_
	HANDLE
	WINAPI
	CreateProxyThread(
		_In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
		_In_ SIZE_T dwStackSize,
		_In_ LPTHREAD_START_ROUTINE lpStartAddress,
		_In_opt_ __drv_aliasesMem LPVOID lpParameter,
		_In_ DWORD dwCreationFlags,
		_Out_opt_ LPDWORD lpThreadId
	);

	void InitializeThreadProxyer();
	void UninitializeThreadProxyer();
}
