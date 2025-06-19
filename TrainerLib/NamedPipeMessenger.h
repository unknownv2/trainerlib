#pragma once

#include <Windows.h>
#include <memory>
#include "IMessenger.h"
#include "ILogger.h"

namespace TrainerLib
{
	class NamedPipeMessenger : public IMessenger
	{
		struct initializing_message final
		{
			uint64_t messageType = 0;
		};

		struct ready_message final
		{
			uint64_t messageType = 1;
		};

		struct value_message final
		{
			uint64_t messageType = 2;
			char varName[32];
			double value;
		};

		std::shared_ptr<ILogger> _logger;
		std::wstring _readPipeName;
		HANDLE _hReadPipe = INVALID_HANDLE_VALUE;
		HANDLE _hWritePipe = INVALID_HANDLE_VALUE;

		bool SendPipeMessage(void* buffer, DWORD bufferSize);

	public:
		NamedPipeMessenger(std::shared_ptr<ILogger> logger, std::wstring readPipeName)
			: _logger(move(logger)), _readPipeName(readPipeName)
		{
			
		}

		bool IsConnected() override;

		bool Connect() override;

		void Disconnect() override;

		bool GetNextValue(std::string& varName, double& value) override;

		bool SendValue(std::string varName, double value) override;

		bool NotifyInitializationStarted() override;

		bool NotifyInitializationIsComplete() override;

		~NamedPipeMessenger() override;
	};
}
