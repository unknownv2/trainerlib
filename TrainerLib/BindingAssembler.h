#pragma once

#include <Windows.h>
#include <memory>
#include <mutex>
#include <map>
#include "IRemoteClient.h"
#include "IAssembler.h"
#include "ILogger.h"

namespace TrainerLib
{
	class BindingAssembler : public IAssembler, IValueChangedHandler, IUndefinedSymbolHandler
	{
		static DWORD WINAPI PollForChanges(void* arg);

		const std::string FloatSuffix = "_f";
		const std::string DoubleSuffix = "_d";
		const std::string FlagSuffix = "_c";

		std::shared_ptr<IRemoteClient> _client;
		std::shared_ptr<ILogger> _logger;
		std::unique_ptr<IAssembler> _assembler;

		std::map<std::string, double> _variables;
		HANDLE _hSymbolPollThread = INVALID_HANDLE_VALUE;

		struct allocated_symbol
		{
			int64_t llValue;
			double dValue;
			float fValue;
			uint32_t cFlag;
		};

		static bool AlmostEqual(double a, double b)
		{
			return fabs(a - b) < std::numeric_limits<double>::epsilon();
		}

		allocated_symbol* SetValueInternal(std::string name, double value, bool setFlag);

		void HandleValueChanged(const char* name, double value) override;

	protected:
		void StartPolling();

	public:
		BindingAssembler(
			std::shared_ptr<IRemoteClient> client,
			std::shared_ptr<ILogger> logger,
			std::unique_ptr<IAssembler> assembler) 
			: _client(move(client)), _logger(move(logger)), _assembler(move(assembler))
		{
			_client->AddValueChangedHandler(this);
			_assembler->SetUndefinedSymbolHandler(this);
		}

		allocated_symbol* AllocateInfinityVar(const char* name) const;

		bool Assemble(const char* script, bool enable) override;

		void* GetSymbolAddress(const char* name) override;

		void SetSymbolAddress(const char* name, void* address) override;

		void* AllocateSymbol(const char* name, size_t size, const wchar_t* nearModule = nullptr) override;

		void* HandleUndefinedSymbol(const char* name) override;

		void PredefineSymbol(const char* name) override;

		void EnableDataScans() override
		{
			_assembler->EnableDataScans();
		}

		void DisableDataScans() override
		{
			_assembler->DisableDataScans();
		}

		void SetUndefinedSymbolHandler(IUndefinedSymbolHandler*) override
		{
			// Ignore this call because we use our own.
		}

		~BindingAssembler() override;
	};
}
