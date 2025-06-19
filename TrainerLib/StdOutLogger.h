#pragma once

#include <string>
#include <iostream>
#include "ILogger.h"

namespace TrainerLib
{
	class StdOutLogger : public ILogger
	{
	protected:
		void LogMessage(LogLevel level, const wchar_t* message) override
		{
			std::wstring str = message;

			if (level == LogLevel::Error) {
				std::wcerr << str;
			}
			else {
				std::wcout << str;
			}
		}

	public:
		bool IsConnected() override
		{
			return true;
		}
	};
}
