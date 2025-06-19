#include <Windows.h>
#include <Psapi.h>
#include <sstream>
#include <iomanip>
#include "Infinity.h"

namespace TrainerLib
{
	using namespace std;

	Infinity::Infinity(shared_ptr<IMessenger> messenger, shared_ptr<ILogger> logger, shared_ptr<UnhandledExceptionCatcher> catcher)
		: _messenger(move(messenger)), _logger(move(logger)), _catcher(move(catcher))
	{
		_catcher->AddErrorHandler(this);
	}

	bool Infinity::Connect()
	{
		_logger->Log(LogLevel::Debug, "Connecting to Infinity...");

		if (!_messenger->Connect()) {
			_logger->Log(LogLevel::Error, "Failed to connect!");
			return false;
		}

		return true;
	}

	void Infinity::RunLoop()
	{
		string name;
		double value;

		while (_messenger->IsConnected() && _messenger->GetNextValue(name, value)) {

			_logger->Log(LogLevel::Debug, "%s set to %lg by client.", name.c_str(), value);

			_values[name] = value;

			for (auto& handler : _changeHandlers) {
				try {
					handler->HandleValueChanged(name.c_str(), value);
					if (_values[name] != value) {
						break;
					}
				}
				catch (...) {
					_logger->Log(LogLevel::Error, "Value handler for %s threw an exception.", name.c_str());
				}
			}
		}
	}

	void Infinity::SetValue(const char* name, double value)
	{
		string nameStr = name;

		_logger->Log(LogLevel::Debug, "%s set to %lg by trainer.", name, value);
		_values[nameStr] = value;
		for (auto& handler : _changeHandlers) {
			try {
				handler->HandleValueChanged(name, value);
				if (_values[nameStr] != value) {
					break;
				}
			}
			catch (...) {
				_logger->Log(LogLevel::Error, "Value handler for %s threw an exception.", name);
			}
		}

		_messenger->SendValue(name, value);
	}

	void Infinity::OnError(DWORD threadId, PEXCEPTION_POINTERS exception)
	{
		auto ex = exception->ExceptionRecord;

		HMODULE hModule;
		auto inModule = GetModuleHandleEx(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, 
			static_cast<LPCTSTR>(ex->ExceptionAddress), 
			&hModule) != FALSE;

		MODULEINFO moduleInfo {};
		if (inModule) {
			inModule = GetModuleInformation(GetCurrentProcess(), hModule, &moduleInfo, sizeof(moduleInfo)) != FALSE;
		}
		
		wchar_t moduleName[MAX_PATH];
		if (inModule) {
			GetModuleBaseNameW(GetCurrentProcess(), hModule, moduleName, MAX_PATH);
		}

		stringstream ss;

		ss << "=== BEGIN EXCEPTION ===" << '\n' << '\n';


		/* Info */
		ss << "--- Info ---" << '\n';
		ss << "Code:       " << ExceptionCodeToString(ex->ExceptionCode) << " (0x" << hex << ex->ExceptionCode << ")" << dec << '\n';
		if (ex->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || ex->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
			ss << "Violation:  " << ViolationType(ex->ExceptionInformation[0]) << " violation at 0x" << hex << ex->ExceptionInformation[1] << dec << '\n';
		}
		if (ex->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
			ss << "NTSTATUS:   0x" << hex << ex->ExceptionRecord->ExceptionInformation[2] << dec << '\n';
		}
		ss << "Address:    0x" << hex << ex->ExceptionAddress << dec << '\n';
		if (inModule) {
			ss << "Module:     " << moduleName << " (0x" << hex << moduleInfo.lpBaseOfDll << dec << ")" << '\n';
			ss << "RVA:        0x" << hex << intptr_t(ex->ExceptionAddress) - intptr_t(moduleInfo.lpBaseOfDll) << dec << '\n';
		} else {
			ss << "Module:     None\n";
		}
		ss << "Thread ID:  " << threadId << '\n';
		ss << "Last Win32: 0x" << hex << GetLastError() << dec << '\n';
		ss << "--- End Info ---" << '\n' << '\n';


		/* Stack Trace */
		/*ss << "--- Stack Trace ---" << '\n';
		ss << "Coming soon" << '\n';
		ss << "--- End Stack Trace ---" << '\n' << '\n';*/


		/* Registers */
		ss << "--- Registers ---" << '\n';
		PrintContext(ss, exception->ContextRecord);
		ss << "--- End Registers ---" << '\n' << '\n';


		/* Trainer State */
		ss << "--- Trainer State ---" << '\n';
		size_t longest = 0;
		for (auto value : _values) {
			if (value.first.length() > longest) {
				longest = value.first.length();
			}
		}
		longest++;
		for (auto value : _values) {
			ss << setw(longest) << left<< setfill(' ') << value.first.c_str() << resetiosflags << ' ' << value.second << '\n';
		}
		ss << "--- End Trainer State ---" << '\n' << '\n';


		ss << "=== END EXCEPTION ===";

		ss.seekp(0);
		string item;
		while (getline(ss, item, '\n')) {
			_logger->Log(LogLevel::Error, item.c_str());
		}
	}

	void Infinity::PrintContext(stringstream& ss, PCONTEXT context)
	{
#define REG(name, var) ss << name << ' ' << hex << setw(sizeof(void*) * 2) << setfill('0') << right << context-> var << dec << resetiosflags << ' ' << context-> var << '\n'

#ifdef _AMD64_
		REG("RIP", Rip);
		REG("RAX", Rax);
		REG("RBX", Rbx);
		REG("RCX", Rcx);
		REG("RDX", Rdx);
		REG("RSP", Rsp);
		REG("RBP", Rbp);
		REG("RSI", Rsi);
		REG("RDI", Rdi);
		REG("R8 ", R8);
		REG("R9 ", R9);
		REG("R10", R10);
		REG("R11", R11);
		REG("R12", R12);
		REG("R13", R13);
		REG("R14", R14);
		REG("R15", R15);
#else
		REG("EAX", Eax);
		REG("EBX", Ebx);
		REG("ECX", Ecx);
		REG("EDX", Edx);
		REG("EDI", Edi);
		REG("ESI", Esi);
		REG("EBP", Ebp);
		REG("EIP", Eip);
		REG("ESP", Esp);
#endif

		REG("DR0", Dr0);
		REG("DR1", Dr1);
		REG("DR2", Dr2);
		REG("DR3", Dr3);
		REG("DR6", Dr6);
		REG("DR7", Dr7);
	}

	const char* Infinity::ExceptionCodeToString(const DWORD& code)
	{
		switch (code) {
		case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
		case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
		case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
		case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
		case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
		case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
		case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
		case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
		case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
		case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
		case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
		case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
		case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
		case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
		case 0xe0434352:						 return "CLR Exception";
		default: return "UNKNOWN";
		}
	}

	const char* Infinity::ViolationType(const ULONG_PTR code)
	{
		switch (code) {
		case 0: return "Read";
		case 1: return "Write";
		case 8: return "DEP";
		default: return "Unknown";
		}
	}
}
