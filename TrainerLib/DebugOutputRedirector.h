#pragma once

#include <memory>
#include "ILogger.h"
#include "IHooker.h"

namespace TrainerLib
{
	class DebugOutputRedirector
	{
		using OutputDebugStringAT = void(WINAPI*)(LPCSTR lpOutputString);
		using OutputDebugStringWT = void(WINAPI*)(LPCWSTR lpOutputString);

		IHook* _ansiHook;
		IHook* _unicodeHook;
		std::shared_ptr<ILogger> _logger;
		static void WINAPI OutputDebugStringADetour(LPCSTR lpOutputString);
		static void WINAPI OutputDebugStringWDetour(LPCWSTR lpOutputString);

	public:
		explicit DebugOutputRedirector(std::shared_ptr<ILogger> logger, std::shared_ptr<IHooker> hooker);
		~DebugOutputRedirector();
	};
}
