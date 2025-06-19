#pragma once

#include <Windows.h>
#include <set>
#include <memory>
#include <map>
#include "IRemoteClient.h"
#include "IMessenger.h"
#include "ILogger.h"
#include "UnhandledExceptionCatcher.h"

namespace TrainerLib
{
	class Infinity : public IRemoteClient, public IErrorHandler
	{
		std::shared_ptr<IMessenger> _messenger;
		std::shared_ptr<ILogger> _logger;
		std::shared_ptr<UnhandledExceptionCatcher> _catcher;
		std::set<uint32_t> _allowedVars;
		std::map<std::string, double> _values;
		std::set<IValueChangedHandler*> _changeHandlers;

		void OnError(DWORD threadId, PEXCEPTION_POINTERS exception) override;

		static const char* ExceptionCodeToString(const DWORD& code);
		static const char* ViolationType(const ULONG_PTR opcode);
		static void PrintContext(std::stringstream& ss, PCONTEXT context);

	public:
		static uint32_t RandomValue;

		Infinity(
			std::shared_ptr<IMessenger> messenger,
			std::shared_ptr<ILogger> logger,
			std::shared_ptr<UnhandledExceptionCatcher> catcher);

		bool IsConnected() override
		{
			return _messenger->IsConnected();
		}

		bool Connect() override;

		void RunLoop() override;

		void Disconnect() override
		{
			_messenger->Disconnect();
		}

		double GetValue(const char* name) override
		{
			auto value = _values.find(name);

			return value == _values.end() ? 0 : value->second;
		}

		void SetValue(const char* name, double value) override;

		void AddValueChangedHandler(IValueChangedHandler* handler) override
		{
			_changeHandlers.insert(handler);
		}

		void RemoveValueChangedHandler(IValueChangedHandler* handler) override
		{
			_changeHandlers.erase(handler);
		}

		~Infinity() override
		{
			_catcher->RemoveErrorHandler(this);
		}
	};
}
