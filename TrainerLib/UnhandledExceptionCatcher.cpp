#include <Windows.h>
#include "UnhandledExceptionCatcher.h"
#include "ThreadProxy.h"

namespace TrainerLib
{
	UnhandledExceptionCatcher* UnhandledExceptionCatcher::_instance = nullptr;

	DWORD WINAPI UnhandledExceptionCatcher::HandleException(LPVOID lpExceptionParams)
	{
		auto instance = _instance;

		if (instance == nullptr) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		auto params = static_cast<exception_params*>(lpExceptionParams);
		auto previous = instance->_previousFilter;

		LONG result;

		if (previous != nullptr) {
			if ((result = previous(params->exceptionInfo)) == EXCEPTION_CONTINUE_EXECUTION) {
				return DWORD(EXCEPTION_CONTINUE_EXECUTION);
			}
		}
		else {
			result = EXCEPTION_CONTINUE_SEARCH;
		}

		for (auto handler : instance->_handlers) {
			handler->OnError(params->threadId, params->exceptionInfo);
		}

		return result;
	}

	LONG WINAPI UnhandledExceptionCatcher::UnhandledExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo)
	{
		exception_params params;
		params.threadId = GetCurrentThreadId();
		params.exceptionInfo = ExceptionInfo;

		DWORD exitCode;
		auto hThread = CreateProxyThread(nullptr, 0, HandleException, &params, 0, &exitCode);

		WaitForSingleObject(hThread, INFINITE);

		if (GetExitCodeThread(hThread, &exitCode) == 0) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		return exitCode;
	}

	LPTOP_LEVEL_EXCEPTION_FILTER WINAPI UnhandledExceptionCatcher::SetUnhandledExceptionFilterDetour(
		LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
	{
		auto instance = _instance;

		if (instance == nullptr) {
			return nullptr;
		}

		if (lpTopLevelExceptionFilter == UnhandledExceptionFilter) {
			return instance->_setUnhandledExceptionFilterOrig(lpTopLevelExceptionFilter);
		}

		auto previous = instance->_previousFilter;
		instance->_previousFilter = lpTopLevelExceptionFilter;
		return previous;
	}

	void UnhandledExceptionCatcher::CreateFilter()
	{
		/*_logger->Log(LogLevel::Debug, "Registering unhandled exception filter...");

		if (_instance != nullptr) {
			_logger->Log(LogLevel::Error, "Cannot initialize an UnhandledExceptionFilter while another one is present!");
			return;
		}

		auto hook = _hooker->CreateHook(
			SetUnhandledExceptionFilter, 
			SetUnhandledExceptionFilterDetour);

		if (hook == nullptr) {
			_logger->Error("Failed to hook SetUnhandledExceptionFilter!");
			return;
		}

		if (!hook->Enable()) {
			_logger->Error("Failed to enable the SetUnhandledExceptionFilter hook!");
			return;
		}

		_hook = hook;
		_setUnhandledExceptionFilterOrig = SetUnhandledExceptionFilterT(hook->OriginalAddress());

		_instance = this;

		_previousFilter = SetUnhandledExceptionFilter(UnhandledExceptionFilter);*/
	}

	UnhandledExceptionCatcher::~UnhandledExceptionCatcher()
	{
		if (_hook != nullptr) {
			//SetUnhandledExceptionFilter(_previousFilter);
			//delete _hook;
		}

		_instance = nullptr;
	}
}
