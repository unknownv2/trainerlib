#include <Windows.h>
#include <tlhelp32.h>
#include <set>
#include <mutex>
#include <vector>
#include <Psapi.h>
#include "Debugger.h"
#include <ThemidaSDK.h>

namespace TrainerLib
{
	int debuggerIntegrityCheck = 2;

	EXTERN_C IMAGE_DOS_HEADER __ImageBase;

	// Dr7 register masks and flags.
	#define HW_BREAKPOINT_LOCAL_FLAG(index) (1ull << (( index * 2)))
	#define HW_BREAKPOINT_GLOBAL_FLAG(index) (HW_BREAKPOINT_LOCAL_FLAG( index ) << 1)
	#define HW_BREAKPOINT_TRIGGER(index, trigger) ((uintptr_t( trigger ) & 3) << (16 + ( index * 4)))
	#define HW_BREAKPOINT_SIZE(index, size) ((uintptr_t( size ) & 3) << (18 + ( index * 4)))
	#define HW_BREAKPOINT_ENABLED(context, index) (( context .Dr7 & HW_BREAKPOINT_GLOBAL_FLAG( index ) != 0)

	#define HW_BREAKPOINT_SET(context, index, address, trigger)	\
	context .Dr##index = uintptr_t( address );					\
	context .Dr7 |= HW_BREAKPOINT_LOCAL_FLAG( index );			\
	context .Dr7 |= HW_BREAKPOINT_SIZE( index , 0 );			\
	context .Dr7 |= HW_BREAKPOINT_TRIGGER( index, trigger )

	#define HW_BREAKPOINT_UNSET(context, index)			\
	context .Dr##index = 0;								\
	context .Dr7 &= ~HW_BREAKPOINT_LOCAL_FLAG( index );	\
	context .Dr7 &= ~HW_BREAKPOINT_SIZE( index , 3 );	\
	context .Dr7 &= ~HW_BREAKPOINT_TRIGGER( index, 3 )

	#define HW_BREAKPOINT_IS_SET(context, index) ((context .Dr7 & HW_BREAKPOINT_LOCAL_FLAG( index )) != 0)

	Debugger* Debugger::_instance = nullptr;

	LONG Debugger::VectoredHandler(PEXCEPTION_POINTERS exceptionInfo)
	{
		// Ignore access violations raised in this library.
		if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
			auto address = uintptr_t(exceptionInfo->ExceptionRecord->ExceptionAddress);
			if (address >= _libStart && address <= _libEnd) {
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}

		_lock.lock();

		auto threadId = GetCurrentThreadId();

		if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP) {
			auto pending = _singleStepQueue.find(threadId);
			if (pending != _singleStepQueue.end()) {
				auto address = pending->second;
				auto breakpoint = _breakpoints.find(address);
				auto result = true;
				if (breakpoint != _breakpoints.end()) {
					// We want to call all the post-breakpoint handlers now.
					_lock.unlock();
					for (auto& handler : _handlers) {
						result = handler->HandleBreakpoint(address, threadId, exceptionInfo->ContextRecord, true);
						if (result) {
							break;
						}
					}
					_lock.lock();
					// Enable the breakpoint again if it wasn't disabled in a post handler.
					if ((breakpoint = _breakpoints.find(address)) != _breakpoints.end()) {
						if (IsHwTrigger(breakpoint->second.eTrigger)) {
							SetHardwareBreakpoint(*exceptionInfo->ContextRecord, breakpoint->second.lpAddress, breakpoint->second.eTrigger, true);
						} else if (breakpoint->second.eTrigger == BreakpointTrigger::Opcode) {
							SetOpcodeBreakpoint(breakpoint->second, true);
						} else if (breakpoint->second.eTrigger == BreakpointTrigger::Dereference) {
							SetDereferenceBreakpoint(breakpoint->second, true);
						}
					}
				}
				exceptionInfo->ContextRecord->Dr6 = 0;
				_singleStepQueue.erase(threadId);
				_lock.unlock();
				return result ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
			}
		}

		auto realBreakpointAddress = exceptionInfo->ExceptionRecord->ExceptionAddress;

		if (exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
			auto addr = intptr_t(exceptionInfo->ExceptionRecord->ExceptionInformation[1]);

			if (addr >= 0) {
				_lock.unlock();
				return EXCEPTION_CONTINUE_SEARCH;
			}

			addr = -addr;

			for (auto& breakpoint : _breakpoints) {
				if (breakpoint.second.eTrigger == BreakpointTrigger::Dereference && breakpoint.second.lpDerefAddress == addr) {
					realBreakpointAddress = breakpoint.second.lpAddress;
					break;
				}
			}

			if (realBreakpointAddress == exceptionInfo->ExceptionRecord->ExceptionAddress) {
				_lock.unlock();
				return EXCEPTION_CONTINUE_SEARCH;
			}
		} else if (exceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_PRIV_INSTRUCTION) {
			// For data breakpoints, we need to get the address of the data, not the exception.
			auto dr6 = exceptionInfo->ContextRecord->Dr6;

			if (dr6 & 1) {
				realBreakpointAddress = reinterpret_cast<void*>(exceptionInfo->ContextRecord->Dr0);
			} else if (dr6 & 2) {
				realBreakpointAddress = reinterpret_cast<void*>(exceptionInfo->ContextRecord->Dr1);
			} else if (dr6 & 4) {
				realBreakpointAddress = reinterpret_cast<void*>(exceptionInfo->ContextRecord->Dr2);
			} else if (dr6 & 8) {
				realBreakpointAddress = reinterpret_cast<void*>(exceptionInfo->ContextRecord->Dr3);
			}
		}

		exceptionInfo->ContextRecord->Dr6 = 0;

		auto breakpoint = _breakpoints.find(realBreakpointAddress);

		// Return if we don't have a record of this breakpoint.
		if (breakpoint == _breakpoints.end()) {
			_lock.unlock();
			return exceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
		}

		auto trigger = breakpoint->second.eTrigger;

		// Disable the breakpoint before calling the handlers.
		// We don't want the breakpoint to be stuck in a loop if a handler triggers it.
		if (IsHwTrigger(trigger)) {
			SetHardwareBreakpoint(*exceptionInfo->ContextRecord, breakpoint->second.lpAddress, trigger, false);
			SetHardwareBreakpoint(GetCurrentThread(), breakpoint->second.lpAddress, trigger, false);
		} else if (trigger == BreakpointTrigger::Opcode) {
			SetOpcodeBreakpoint(breakpoint->second, false);
		} else if (trigger == BreakpointTrigger::Dereference) {
			// We have to make sure no registers are still referencing
			// the bad address. Otherwise we'll be in an access violation loop.
			auto badAddress = uintptr_t(exceptionInfo->ExceptionRecord->ExceptionInformation[1]);
			auto goodAddress = uintptr_t(breakpoint->second.lpDerefAddress);
			SetDereferenceBreakpoint(breakpoint->second, false);
			auto& context = *exceptionInfo->ContextRecord;
#define CheckReg(reg) if (context.reg == badAddress) context.reg = goodAddress
#ifdef _AMD64_
			CheckReg(Rax);
			CheckReg(Rcx);
			CheckReg(Rdx);
			CheckReg(Rbx);
			CheckReg(Rsp);
			CheckReg(Rbp);
			CheckReg(Rsi);
			CheckReg(Rdi);
			CheckReg(R8);
			CheckReg(R9);
			CheckReg(R10);
			CheckReg(R11);
			CheckReg(R12);
			CheckReg(R13);
			CheckReg(R14);
			CheckReg(R15);
#else
			CheckReg(Edi);
			CheckReg(Esi);
			CheckReg(Ebx);
			CheckReg(Edx);
			CheckReg(Ecx);
			CheckReg(Eax);
#endif
#undef CheckReg
		}

		auto result = true;

		_lock.unlock();

		// Call all the breakpoint handlers.
		for (auto& handler : _handlers) {
			result = handler->HandleBreakpoint(realBreakpointAddress, threadId, exceptionInfo->ContextRecord, false);
			if (result) {
				break;
			}
		}

		_lock.lock();

		// Make sure the breakpoint still exists. The handler may have deleted it.
		breakpoint = _breakpoints.find(realBreakpointAddress);

		// If the breakpoint still exists, set up a single step so we can enable it again.
		if (breakpoint != _breakpoints.end()) {
			_singleStepQueue[threadId] = breakpoint->second.lpAddress;
			exceptionInfo->ContextRecord->EFlags |= 0x100;
		}

		_lock.unlock();

		return result ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_CONTINUE_SEARCH;
	}
	
	LONG CALLBACK Debugger::VectoredHandlerStatic(PEXCEPTION_POINTERS exceptionInfo)
	{
		auto code = exceptionInfo->ExceptionRecord->ExceptionCode;

		// Ignore exceptions that we can't handle.
		if (code != EXCEPTION_ACCESS_VIOLATION &&
			code != EXCEPTION_PRIV_INSTRUCTION &&
			code != EXCEPTION_BREAKPOINT &&
			code != EXCEPTION_SINGLE_STEP &&
			code != STATUS_WX86_SINGLE_STEP) {
			return EXCEPTION_CONTINUE_SEARCH;
		}
		
		auto instance = _instance;
		if (instance != nullptr) {
			return instance->VectoredHandler(exceptionInfo);
		}

		return EXCEPTION_CONTINUE_SEARCH;
	}

	bool Debugger::Init()
	{
		_lock.lock();

		if (_hExceptionHandler != nullptr) {
			_lock.unlock();
			return true;
		}

		_hExceptionHandler = AddVectoredExceptionHandler(CALL_FIRST, VectoredHandlerStatic);

		_lock.unlock();

		if (_hExceptionHandler == nullptr) {
			_logger->Log(LogLevel::Error, L"AddVectoredExceptionHandler() failed during debugger initialization.");
			return false;
		}

		VM_FAST_START

		CHECK_PROTECTION(debuggerIntegrityCheck, 2)

		MODULEINFO modInfo;

		if (!GetModuleInformation(GetCurrentProcess(), HMODULE(&__ImageBase), &modInfo, sizeof(modInfo))) {
			_logger->Log(LogLevel::Error, L"GetModuleInformation() failed during debugger initialization.");
			return false;
		}

		if (debuggerIntegrityCheck == 2) {
			CHECK_CODE_INTEGRITY(debuggerIntegrityCheck, 4);
		}

		_libStart = uintptr_t(modInfo.lpBaseOfDll);
		_libEnd = _libStart + modInfo.SizeOfImage;

		_logger->Log(LogLevel::Debug, "Debugger initialized.");

		VM_FAST_END

		return true;
	}

	void Debugger::AddBreakpointHandler(IBreakpointHandler* handler)
	{
		_handlers.insert(handler);
	}

	void Debugger::RemoveBreakpointHandler(IBreakpointHandler* handler)
	{
		_handlers.erase(handler);
	}

	void Debugger::ClearOpcodeBreakpoint(Breakpoint& breakpoint)
	{
		auto ptr = static_cast<uint8_t*>(breakpoint.lpAddress);

		if (*ptr == EXCEPTION_OPCODE) {
			DWORD dwOldProtect;
			VirtualProtect(breakpoint.lpAddress, 1, PAGE_READWRITE, &dwOldProtect);

			*ptr = breakpoint.bPreviousInstruction;
			FlushInstructionCache(GetCurrentProcess(), breakpoint.lpAddress, 1);

			DWORD dwNewProtect;
			VirtualProtect(breakpoint.lpAddress, 1, dwOldProtect, &dwNewProtect);
		}

		breakpoint.bPreviousInstruction = 0;
	}

	void Debugger::SetHardwareBreakpoint(HANDLE hThread, void* address, BreakpointTrigger trigger, bool enable)
	{
		CONTEXT context;
		context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		if (!GetThreadContext(hThread, &context)) {
			return;
		}

		SetHardwareBreakpoint(context, address, trigger, enable);

		SetThreadContext(hThread, &context);
	}

	void Debugger::SetHardwareBreakpoint(CONTEXT& context, void* address, BreakpointTrigger trigger, bool enable)
	{
		auto addressPtr = uintptr_t(address);

		auto bTrigger = HwTriggerValue(trigger);

		if (enable) {
			if (!HW_BREAKPOINT_IS_SET(context, 0)) {
				HW_BREAKPOINT_SET(context, 0, address, bTrigger);
			} else if (!HW_BREAKPOINT_IS_SET(context, 1)) {
				HW_BREAKPOINT_SET(context, 1, address, bTrigger);
			} else if (!HW_BREAKPOINT_IS_SET(context, 2)) {
				HW_BREAKPOINT_SET(context, 2, address, bTrigger);
			} else if (!HW_BREAKPOINT_IS_SET(context, 3)) {
				HW_BREAKPOINT_SET(context, 3, address, bTrigger);
			}
		} else {
			if (context.Dr0 == addressPtr && HW_BREAKPOINT_IS_SET(context, 0)) {
				HW_BREAKPOINT_UNSET(context, 0);
			} else if (context.Dr1 == addressPtr && HW_BREAKPOINT_IS_SET(context, 1)) {
				HW_BREAKPOINT_UNSET(context, 1);
			} else if (context.Dr2 == addressPtr && HW_BREAKPOINT_IS_SET(context, 2)) {
				HW_BREAKPOINT_UNSET(context, 2);
			} else if (context.Dr3 == addressPtr && HW_BREAKPOINT_IS_SET(context, 3)) {
				HW_BREAKPOINT_UNSET(context, 3);
			}
		}
	}

	bool Debugger::SetPreferredHardwareBreakpoints(HANDLE hThread)
	{
		CONTEXT context;
		context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

		if (!GetThreadContext(hThread, &context)) {
			_logger->Log(LogLevel::Warning, "Failed to get thread context of thread ID %d.", GetThreadId(hThread));
			return false;
		}

		// Disable all breakpoints.
		context.Dr7 = 0;

		auto x = 0;

		// The first pass will only enable data breakpoints.
		// The second pass will only enable execute breakpoints.
		// Up to 4 breakpoints can be set, combined.
		for (auto i = 0; i < 2 && x < 4; i++) {
			for (auto& breakpoint : _breakpoints) {
				if (i == 0 && IsDataTrigger(breakpoint.second.ePreferredTrigger) ||
					i == 1 && breakpoint.second.ePreferredTrigger == BreakpointTrigger::Execute) {

					// If this was previously another type of breakpoint, we have to clear it.
					if (breakpoint.second.eTrigger == BreakpointTrigger::Opcode) {
						ClearOpcodeBreakpoint(breakpoint.second);
					} else if (breakpoint.second.eTrigger == BreakpointTrigger::Dereference) {
						SetDereferenceBreakpoint(breakpoint.second, false);
					}

					auto bTrigger = HwTriggerValue(breakpoint.second.ePreferredTrigger);

					if (x == 0) {
						HW_BREAKPOINT_SET(context, 0, breakpoint.first, bTrigger);
					} else if (x == 1) {
						HW_BREAKPOINT_SET(context, 1, breakpoint.first, bTrigger);
					} else if (x == 2) {
						HW_BREAKPOINT_SET(context, 2, breakpoint.first, bTrigger);
					} else {
						HW_BREAKPOINT_SET(context, 3, breakpoint.first, bTrigger);
					}

					breakpoint.second.eTrigger = breakpoint.second.ePreferredTrigger;

					++x;

					if (x == 4) {
						break;
					}
				}
			}
		}

		if (SetThreadContext(hThread, &context) == 0) {
			_logger->Log(LogLevel::Warning, "Failed to set thread context of thread ID %d.", GetThreadId(hThread));
			return false;
		}

		return true;
	}

	void Debugger::SetOpcodeBreakpoint(Breakpoint& breakpoint, bool enable) const
	{
		DWORD dwOldProtect;
		if (!VirtualProtect(breakpoint.lpAddress, 1, PAGE_READWRITE, &dwOldProtect)) {
			_logger->Error("VirtualProtect failed while setting software breakpoint.");
		}

		auto ptr = static_cast<uint8_t*>(breakpoint.lpAddress);

		if (enable) {
			breakpoint.bPreviousInstruction = *ptr;
			*ptr = EXCEPTION_OPCODE;
		} else {
			*ptr = breakpoint.bPreviousInstruction;
		}

		DWORD dwNewProtect;
		VirtualProtect(breakpoint.lpAddress, 1, dwOldProtect, &dwNewProtect);

		FlushInstructionCache(GetCurrentProcess(), breakpoint.lpAddress, 1);

		breakpoint.eTrigger = BreakpointTrigger::Opcode;
	}

	void Debugger::SetDereferenceBreakpoint(Breakpoint& breakpoint, bool enable)
	{
		try {
			// lpAddress is a pointer to a memory address
			// that holds a pointer the CPU will dereference.
			auto ptr = static_cast<volatile intptr_t*>(breakpoint.lpAddress);

			// Which address does it point to?
			// The VEH will ignore an invalid pointer exception here.
			auto addr = *ptr;

			// If the address is in user space, the "breakpoint" is disabled.
			if (intptr_t(addr) >= 0) {
				if (enable) {
					// Negate the pointer so it points to privleged kernel space.
					breakpoint.lpDerefAddress = addr;
					addr = -addr;
				} else {
					return;
				}
			} else if (!enable) {
				// Negate the pointer back to its original value.
				addr = -addr;
				breakpoint.lpDerefAddress = addr;
			} else {
				return;
			}

			// Update the pointer in memory.
			*ptr = addr;
		} catch (...) {

		}

		breakpoint.eTrigger = BreakpointTrigger::Dereference;
	}

	void Debugger::SetAllBreakpoints()
	{
		auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

		if (hSnapshot == INVALID_HANDLE_VALUE) {
			_logger->Log(LogLevel::Error, L"CreateToolhelp32Snapshot failed while setting breakpoint.");
			return;
		}

		auto dwProcId = GetCurrentProcessId();

		std::vector<HANDLE> threads;

		THREADENTRY32 te;
		te.dwSize = sizeof(te);

		// Enumerate all the threads on the system.
		// Open the ones that belong to this process.
		while (Thread32Next(hSnapshot, &te)) {
			if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID)) {
				if (te.th32OwnerProcessID == dwProcId) {
					threads.push_back(OpenThread(THREAD_QUERY_LIMITED_INFORMATION | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, FALSE, te.th32ThreadID));
				}
			}
			te.dwSize = sizeof(te);
		}

		CloseHandle(hSnapshot);

		auto dataBreakpoints = 0;

		for (auto& breakpoint : _breakpoints) {
			if (IsDataTrigger(breakpoint.second.ePreferredTrigger)) {
				dataBreakpoints++;
			}
		}

		if (dataBreakpoints > 4) {
			_logger->Error("Attempted to set more than 4 data breakpoints. The rest will be ignored.");
		}

		auto dwThreadId = GetCurrentThreadId();
		
		{
			// Suspend all threads because we're editing breakpoints.
			for (auto& hThread : threads) {
				if (GetThreadId(hThread) != dwThreadId) {
					SuspendThread(hThread);
				}
			}
		}

		// Set hardware breakpoints on all threads.
		for (auto& hThread : threads) {
			SetPreferredHardwareBreakpoints(hThread);
		}

		auto hwBreakpoints = dataBreakpoints;

		// Set the rest of the breakpoints.
		for (auto& breakpoint : _breakpoints) {

			// Ignore data breakpoints because they were already set.
			if (IsDataTrigger(breakpoint.second.ePreferredTrigger)) {
				continue;
			}

			// Ignore execute breakpoints if they were already set.
			if (hwBreakpoints < 4 && breakpoint.second.ePreferredTrigger == BreakpointTrigger::Execute) {
				hwBreakpoints++;
				continue;
			}

			if (breakpoint.second.ePreferredTrigger == BreakpointTrigger::Opcode) {
				SetOpcodeBreakpoint(breakpoint.second, true);
			} else if (breakpoint.second.ePreferredTrigger == BreakpointTrigger::Dereference) {
				SetDereferenceBreakpoint(breakpoint.second, true);
			} else {
				_logger->Log(LogLevel::Error, "Invalid breakpoint trigger %d.", uint32_t(breakpoint.second.eTrigger));
			}
		}

		VM_FAST_START

		// Resume all the the threads.
		for (auto& hThread : threads) {
			if (debuggerIntegrityCheck == 4 && GetThreadId(hThread) != dwThreadId) {
				ResumeThread(hThread);
			}
			
			CloseHandle(hThread);
		}

		VM_FAST_END
	}

	Debugger::~Debugger()
	{
		_lock.lock();

		if (_hExceptionHandler == nullptr) {
			_lock.unlock();
			return;
		}

		_logger->Log(LogLevel::Debug, "Shutting down debugger...");

		RemoveVectoredExceptionHandler(_hExceptionHandler);

		if (_instance == this) {
			_instance = nullptr;
		}

		_breakpoints.clear();

		SetAllBreakpoints();

		_lock.unlock();
	}

	bool Debugger::IsBreakpointSet(void* lpAddress, BreakpointTrigger* outTrigger)
	{
		auto breakpoint = _breakpoints.find(lpAddress);

		if (breakpoint == _breakpoints.end()) {
			return false;
		}

		if (outTrigger != nullptr) {
			*outTrigger = breakpoint->second.eTrigger;
		}

		return true;
	}

	void Debugger::RegisterThread(HANDLE hThread)
	{
		SetPreferredHardwareBreakpoints(hThread);
	}

	bool Debugger::SetBreakpoint(void* lpAddress, BreakpointTrigger ePreferredTrigger)
	{
		if (_hExceptionHandler == nullptr && !Init()) {
			return false;
		}

		_lock.lock();

		auto existing = _breakpoints.find(lpAddress);
		if (existing != _breakpoints.end()) {
			auto oldTrigger = existing->second.ePreferredTrigger;
			existing->second.ePreferredTrigger = ePreferredTrigger;
			if (ePreferredTrigger != oldTrigger) {
				SetAllBreakpoints();
			}
			_lock.unlock();
			return true;
		}

		Breakpoint breakpoint;
		breakpoint.lpAddress = lpAddress;
		breakpoint.ePreferredTrigger = ePreferredTrigger;
		breakpoint.eTrigger = BreakpointTrigger::None;
		breakpoint.bPreviousInstruction = 0;
		breakpoint.lpDerefAddress = 0;

		_breakpoints.insert({ lpAddress, breakpoint });

		SetAllBreakpoints();

		_lock.unlock();

		return true;
	}

	bool Debugger::UnsetBreakpoint(void* lpAddress)
	{
		if (_hExceptionHandler == nullptr && !Init()) {
			return false;
		}

		_lock.lock();

		auto breakpoint = _breakpoints.find(lpAddress);

		if (breakpoint == _breakpoints.end()) {
			_lock.unlock();
			return false;
		}

		if (IsHwTrigger(breakpoint->second.eTrigger)) {
			_breakpoints.erase(lpAddress);
			SetAllBreakpoints();
		} else if (breakpoint->second.eTrigger == BreakpointTrigger::Opcode) {
			ClearOpcodeBreakpoint(breakpoint->second);
			_breakpoints.erase(lpAddress);
		} else if (breakpoint->second.eTrigger == BreakpointTrigger::Dereference) {
			SetDereferenceBreakpoint(breakpoint->second, false);
			_breakpoints.erase(lpAddress);
		}

		_lock.unlock();

		return true;
	}
}
