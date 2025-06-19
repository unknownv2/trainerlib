#include "NamedPipeLogger.h"

namespace TrainerLib
{
	NamedPipeLogger::NamedPipeLogger(std::wstring pipeName)
		: _pipeName(pipeName), _hPipe(INVALID_HANDLE_VALUE)
	{

	}

	std::wstring NamedPipeLogger::GetPipeName() const
	{
		return _pipeName;
	}

	bool NamedPipeLogger::IsConnected()
	{
		return _hPipe != INVALID_HANDLE_VALUE;
	}

	void NamedPipeLogger::Disconnect()
	{
		if (IsConnected()) {
			auto hPipe = _hPipe;
			_hPipe = INVALID_HANDLE_VALUE;
			CloseHandle(hPipe);
		}
	}

	bool NamedPipeLogger::ConnectIfNotConnected()
	{
		if (IsConnected()) {
			return true;
		}

		_hPipe = CreateFile(
			_pipeName.c_str(),
			GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr);

		return IsConnected();
	}

	void NamedPipeLogger::LogMessage(LogLevel level, const wchar_t* message)
	{
		if (!ConnectIfNotConnected()) {
			return;
		}

		log_message msg;
		msg.dwLevel = static_cast<DWORD>(level);
		msg.dwPadding = 0;
		msg.dwPadding2 = 0;

		auto str = std::wstring(message);
		str._Copy_s(msg.lpwsText, 4096, str.length());
		
		msg.dwMessageLength = static_cast<DWORD>(str.length()) * 2;

		DWORD dwWritten;

		if (!WriteFile(
			_hPipe,
			&msg,
			8 + 8 + static_cast<DWORD>(str.length()) * 2,
			&dwWritten,
			nullptr))
		{
			Disconnect();
		}
	}

	NamedPipeLogger::~NamedPipeLogger()
	{
		Disconnect();
	}
}
