#include "DebugOutputRedirector.h"

namespace TrainerLib
{
	DebugOutputRedirector* _instance;

	DebugOutputRedirector::DebugOutputRedirector(std::shared_ptr<ILogger> logger, std::shared_ptr<IHooker> hooker)
		: _logger(move(logger))
	{
		_instance = this;
		auto kernel32 = GetModuleHandleA("kernel32.dll");
		hooker->BeginTransaction();
		_ansiHook = hooker->CreateHook(GetProcAddress(kernel32, "OutputDebugStringA"), OutputDebugStringADetour);
		_ansiHook->Enable();
		_unicodeHook = hooker->CreateHook(GetProcAddress(kernel32, "OutputDebugStringW"), OutputDebugStringWDetour);
		_unicodeHook->Enable();
		hooker->CommitTransaction();
	}

	DebugOutputRedirector::~DebugOutputRedirector()
	{
		_ansiHook->Remove();
		_unicodeHook->Remove();
	}

	void WINAPI DebugOutputRedirector::OutputDebugStringADetour(LPCSTR lpOutputString)
	{
		if (_instance) {
			auto instance = _instance;
			if (lpOutputString) {
				instance->_logger->Log(LogLevel::Debug, lpOutputString);;
			}
			OutputDebugStringAT(instance->_ansiHook->OriginalAddress())(lpOutputString);
		}
	}

	void WINAPI DebugOutputRedirector::OutputDebugStringWDetour(LPCWSTR lpOutputString)
	{
		if (_instance) {
			auto instance = _instance;
			if (lpOutputString) {
				instance->_logger->Log(LogLevel::Debug, lpOutputString);
			}
			OutputDebugStringWT(instance->_ansiHook->OriginalAddress())(lpOutputString);
		}
	}
}
