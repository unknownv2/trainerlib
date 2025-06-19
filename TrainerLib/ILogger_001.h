#pragma once

#include <cstdint>

namespace TrainerLib
{
	enum class LogLevel : uint8_t
	{
		Debug = 0,
		Info = 1,
		Warning = 2,
		Error = 3,
	};

	class ILogger_001
	{
	public:
		virtual bool IsConnected() = 0;
		virtual void Log(LogLevel level, const wchar_t* message) = 0;
	};

	static const char* const ILogger_001_Version = "ILogger_001";
}
