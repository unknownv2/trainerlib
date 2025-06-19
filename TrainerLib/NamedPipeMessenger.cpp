#include "stdafx.h"
#include "NamedPipeMessenger.h"

namespace TrainerLib
{
	bool NamedPipeMessenger::IsConnected()
	{
		return _hReadPipe != INVALID_HANDLE_VALUE && _hWritePipe != INVALID_HANDLE_VALUE;
	}

	bool NamedPipeMessenger::Connect()
	{
		if (IsConnected()) {
			return true;
		}

		_hReadPipe = CreateFile(
			_readPipeName.c_str(),
			GENERIC_READ,
			0,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr);

		if (_hReadPipe == INVALID_HANDLE_VALUE) {
			_logger->Log(LogLevel::Error, L"Could not connect to Infinity.");
			return false;
		}

		WCHAR writePipeName[MAX_PATH];
		DWORD read;

		auto fSuccess = ReadFile(
			_hReadPipe,
			writePipeName,
			sizeof(writePipeName),
			&read,
			nullptr);

		if (!fSuccess) {
			Disconnect();
			_logger->Log(LogLevel::Error, L"Could not connect to second pipe.");
			return false;
		}

		read /= sizeof(WCHAR);

		writePipeName[read >= MAX_PATH ? MAX_PATH - 1 : read] = 0;

		_hWritePipe = CreateFile(
			writePipeName,
			GENERIC_WRITE,
			0,
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr);

		if (!IsConnected()) {
			return false;
		}

		return true;
	}

	void NamedPipeMessenger::Disconnect()
	{
		if (_hReadPipe != INVALID_HANDLE_VALUE) {
			auto hPipe = _hReadPipe;
			_hReadPipe = INVALID_HANDLE_VALUE;
			CloseHandle(hPipe);
		}

		if (_hWritePipe != INVALID_HANDLE_VALUE) {
			auto hPipe = _hWritePipe;
			_hWritePipe = INVALID_HANDLE_VALUE;
			CloseHandle(hPipe);
		}
	}

	bool NamedPipeMessenger::GetNextValue(std::string& varName, double& value)
	{
		if (!IsConnected()) {
			return false;
		}

		value_message message;
		DWORD read;

		auto fSuccess = ReadFile(
			_hReadPipe,
			&message,
			sizeof(message),
			&read,
			nullptr);

		if (!IsConnected()) {
			return false;
		}

		if (!fSuccess || message.messageType != 2)
		{
			Disconnect();
			return false;
		}

		varName = std::string(message.varName, 0, sizeof(message.varName));
		value = message.value;

		return true;
	}

	bool NamedPipeMessenger::SendPipeMessage(void* buffer, DWORD bufferSize)
	{
		if (!IsConnected()) {
			return false;
		}

		DWORD written;

		if (!WriteFile(
			_hWritePipe,
			buffer,
			bufferSize,
			&written,
			nullptr))
		{
			Disconnect();
			return false;
		}

		return true;
	}

	bool NamedPipeMessenger::NotifyInitializationStarted()
	{
		initializing_message message;

		return SendPipeMessage(&message, sizeof(message));
	}

	bool NamedPipeMessenger::NotifyInitializationIsComplete()
	{
		ready_message message;

		return SendPipeMessage(&message, sizeof(message));
	}

	bool NamedPipeMessenger::SendValue(std::string varName, double value)
	{
		value_message message;
		memset(message.varName, 0, sizeof(message.varName));
		varName._Copy_s(message.varName, sizeof(message.varName), varName.length());
		message.value = value;

		return SendPipeMessage(&message, sizeof(message));
	}

	NamedPipeMessenger::~NamedPipeMessenger()
	{
		NamedPipeMessenger::Disconnect();
	}
}
