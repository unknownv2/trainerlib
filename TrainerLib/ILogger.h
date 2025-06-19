#pragma once

#include <string>
#include "ILogger_001.h"
#include <cstdarg>
#include <Windows.h>

namespace TrainerLib
{
	class ILogger : public ILogger_001
	{
	protected:
		virtual void LogMessage(LogLevel level, const wchar_t* message) = 0;

	public:
		void Log(LogLevel level, const wchar_t* message) override
		{
			LogMessage(level, message);
		}

		void Log(std::wstring message, ...)
		{
			va_list va;
			va_start(va, message);

			wchar_t szMessage[1024];
			vswprintf_s(szMessage, message.c_str(), va);
			szMessage[1023] = 0;

			Log(LogLevel::Info, szMessage);
		}

		void Log(LogLevel level, std::wstring message, ...)
		{
			va_list va;
			va_start(va, message);

			wchar_t szMessage[1024];
			vswprintf_s(szMessage, message.c_str(), va);
			szMessage[1023] = 0;

			Log(level, szMessage);
		}

		void Log(std::string message, ...)
		{
			va_list va;
			va_start(va, message);

			char szMessage[2048];
			vsprintf_s(szMessage, message.c_str(), va);
			szMessage[2047] = 0;

			message = std::string(szMessage);

			Log(LogLevel::Info, std::wstring(message.begin(), message.end()).c_str());
		}

		void Log(LogLevel level, std::string message, ...)
		{
			va_list va;
			va_start(va, message);

			char szMessage[2048];
			vsprintf_s(szMessage, message.c_str(), va);
			szMessage[2047] = 0;

			message = std::string(szMessage);

			Log(level, std::wstring(message.begin(), message.end()).c_str());
		}

		virtual ~ILogger() {}

		void Info(const char* text)
		{
			Log(LogLevel::Debug, text);
		}

		void Warn(const char* text)
		{
			Log(LogLevel::Warning, text);
		}

		void Error(const char* text)
		{
			Log(LogLevel::Error, text);
		}

		void Debug(const char* text)
		{
//#ifdef _DEBUG
			Log(LogLevel::Debug, text);
//#endif
		}
	};
}
