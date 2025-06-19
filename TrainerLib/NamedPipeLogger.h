#pragma once

#include <string>
#include "stdafx.h"
#include "ILogger_001.h"
#include "ILogger.h"

namespace TrainerLib
{
	struct log_message
	{
		DWORD dwLevel;
		DWORD dwPadding;
		DWORD dwMessageLength;
		DWORD dwPadding2;
		WCHAR lpwsText[4096];
	};

	class NamedPipeLogger : public ILogger
	{
		std::wstring _pipeName;
		HANDLE _hPipe;

		bool ConnectIfNotConnected();

	protected:
		void LogMessage(LogLevel level, const wchar_t* message) override;

	public:
		explicit NamedPipeLogger(std::wstring pipeName);

		std::wstring GetPipeName() const;

		bool IsConnected() override;

		void Disconnect();

		~NamedPipeLogger() override;
	};
}
