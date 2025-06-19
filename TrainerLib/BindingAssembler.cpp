#include "BindingAssembler.h"
#include "ThreadProxy.h"
#include <ThemidaSDK.h>

namespace TrainerLib
{
	int ValuePollInterval = 9074;

	DWORD WINAPI BindingAssembler::PollForChanges(void* arg)
	{
		auto assembler = static_cast<BindingAssembler*>(arg);
		
		// Prevent race condition with GetSymbolAddress() attempting to start the poller thread again.
		while (assembler->_hSymbolPollThread == INVALID_HANDLE_VALUE) {
			Sleep(500);
		}

		assembler->_logger->Log(LogLevel::Debug, "Variable polling started.");

		while (true) {
			for (auto& variable : assembler->_variables) {

				auto sym = static_cast<allocated_symbol*>(assembler->GetSymbolAddress(variable.first.c_str()));

				if (sym == nullptr) {
					continue;
				}

				if (sym->llValue != static_cast<int64_t>(variable.second)) {
					sym->dValue = static_cast<double>(sym->llValue);
					assembler->_client->SetValue(variable.first.c_str(), sym->dValue);
					continue;
				}

				if (!AlmostEqual(sym->dValue, variable.second)) {
					assembler->_client->SetValue(variable.first.c_str(), sym->dValue);
					continue;
				}

				if (!AlmostEqual(sym->fValue, static_cast<float>(variable.second))) {
					sym->dValue = static_cast<double>(sym->fValue);
					assembler->_client->SetValue(variable.first.c_str(), sym->dValue);
				}
			}

			Sleep(ValuePollInterval);
		}

		return 0;
	}

	void BindingAssembler::HandleValueChanged(const char* name, double value)
	{
		if (_hSymbolPollThread == INVALID_HANDLE_VALUE) {
			_variables[name] = value;
		} else {
			SetValueInternal(name, value, true);
		}
	}

	BindingAssembler::allocated_symbol* BindingAssembler::SetValueInternal(std::string name, double value, bool setFlag)
	{
		auto sym = static_cast<allocated_symbol*>(_assembler->GetSymbolAddress(name.c_str()));

		if (sym == nullptr) {
			sym = AllocateInfinityVar(name.c_str());
		}
		
		if (setFlag && !AlmostEqual(value, sym->dValue)) {
			sym->cFlag = 1;
		}

		sym->dValue = value;
		sym->fValue = static_cast<float>(value);
		sym->llValue = static_cast<int64_t>(value);

		_variables[name] = value;

		return sym;
	}

	void BindingAssembler::StartPolling()
	{
		_logger->Log(LogLevel::Debug, "Allocating %d variable%s...", _variables.size(), _variables.size() == 1 ? "" : "s");

		VM_FAST_START

		for (auto& variable : _variables) {
			auto sym = AllocateInfinityVar(variable.first.c_str());
			sym->dValue = variable.second;
			sym->fValue = static_cast<float>(variable.second);
			sym->llValue = static_cast<int64_t>(variable.second);
			sym->cFlag = 0;
		}

		_logger->Log(LogLevel::Debug, "Done allocating variables.");

		CHECK_PROTECTION(ValuePollInterval, 11763)

		if (ValuePollInterval == 11763) {

			auto hPollingThread = INVALID_HANDLE_VALUE;

			if (_hSymbolPollThread == INVALID_HANDLE_VALUE) {
				DWORD threadId;
				hPollingThread = CreateProxyThread(nullptr, 0, PollForChanges, this, 0, &threadId);
			}

			CHECK_CODE_INTEGRITY(ValuePollInterval, 250)

			_hSymbolPollThread = hPollingThread;
		}

		VM_FAST_END
	}

	BindingAssembler::allocated_symbol* BindingAssembler::AllocateInfinityVar(const char* name) const
	{
		auto sym = static_cast<allocated_symbol*>(_assembler->AllocateSymbol(name, sizeof allocated_symbol));
		_assembler->SetSymbolAddress((name + FloatSuffix).c_str(), &sym->fValue);
		_assembler->SetSymbolAddress((name + DoubleSuffix).c_str(), &sym->dValue);
		_assembler->SetSymbolAddress((name + FlagSuffix).c_str(), &sym->cFlag);
		return sym;
	}

	bool BindingAssembler::Assemble(const char* script, bool enable)
	{
		if (_hSymbolPollThread == INVALID_HANDLE_VALUE) {
			StartPolling();
		}

		return _assembler->Assemble(script, enable);
	}

	void* BindingAssembler::GetSymbolAddress(const char* name)
	{
		if (_hSymbolPollThread == INVALID_HANDLE_VALUE) {
			StartPolling();
		}

		return _assembler->GetSymbolAddress(name);
	}

	void BindingAssembler::SetSymbolAddress(const char* name, void* address)
	{
		if (_hSymbolPollThread == INVALID_HANDLE_VALUE) {
			StartPolling();
		}

		_assembler->SetSymbolAddress(name, address);
	}

	void* BindingAssembler::AllocateSymbol(const char* name, size_t size, const wchar_t* nearModule)
	{
		return this->_assembler->AllocateSymbol(name, size, nearModule);
	}

	void* BindingAssembler::HandleUndefinedSymbol(const char* name)
	{
		std::string symbol = name;

		// Is this referencing a float variable?
		if (symbol.size() > FloatSuffix.size() && equal(FloatSuffix.rbegin(), FloatSuffix.rend(), symbol.rbegin())) {
			return &SetValueInternal(symbol.erase(symbol.length() - FloatSuffix.size()), 0, false)->fValue;
		}

		// Or a double?
		if (symbol.size() > DoubleSuffix.size() && equal(DoubleSuffix.rbegin(), DoubleSuffix.rend(), symbol.rbegin())) {
			return &SetValueInternal(symbol.erase(symbol.length() - DoubleSuffix.size()), 0, false)->dValue;
		}

		// Or a change flag?
		if (symbol.size() > FlagSuffix.size() && equal(FlagSuffix.rbegin(), FlagSuffix.rend(), symbol.rbegin())) {
			return &SetValueInternal(symbol.erase(symbol.length() - FlagSuffix.size()), 0, false)->cFlag;
		}

		// Use integer by default.
		return &SetValueInternal(symbol, 0, false)->llValue;
	}

	void BindingAssembler::PredefineSymbol(const char* name)
	{
		if (_hSymbolPollThread == INVALID_HANDLE_VALUE && _variables.find(name) == _variables.end()) {
			_variables[name] = 0;
		}

		_assembler->PredefineSymbol(name);
	}

	BindingAssembler::~BindingAssembler()
	{
		if (_hSymbolPollThread != INVALID_HANDLE_VALUE) {
			TerminateThread(_hSymbolPollThread, 0);
		}

		_client->RemoveValueChangedHandler(this);
	}
}
