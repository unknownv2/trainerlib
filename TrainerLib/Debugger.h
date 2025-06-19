#pragma once

#include <map>
#include <set>
#include <stdint.h>
#include "IDebugger_001.h"
#include "ILogger.h"

namespace TrainerLib
{
	#define CALL_FIRST 1  
	#define CALL_LAST 0

	#define STATUS_WX86_SINGLE_STEP 0x4000001e
	#define EFLAG_RESUME 0x10000

	#define EXCEPTION_OPCODE 0xf4

	class Debugger : public IDebugger_001
	{
	public:
		struct Breakpoint
		{
			void* lpAddress;
			intptr_t lpDerefAddress;
			uint8_t bPreviousInstruction;
			BreakpointTrigger eTrigger;
			BreakpointTrigger ePreferredTrigger;
		};

	private:
		static Debugger* _instance;
		static LONG CALLBACK VectoredHandlerStatic(PEXCEPTION_POINTERS exceptionInfo);
		LONG VectoredHandler(PEXCEPTION_POINTERS exceptionInfo);

		std::mutex _lock;
		std::shared_ptr<ILogger> _logger;
		void* _hExceptionHandler = nullptr;
		std::map<uint32_t, void*> _singleStepQueue;
		std::map<void*, Breakpoint> _breakpoints;
		std::set<IBreakpointHandler*> _handlers;

		uintptr_t _libStart;
		uintptr_t _libEnd;

		bool Init();
		static void SetDereferenceBreakpoint(Breakpoint& breakpoint, bool enable);
		static void SetHardwareBreakpoint(HANDLE hThread, void* address, BreakpointTrigger trigger, bool enable);
		static void SetHardwareBreakpoint(CONTEXT& context, void* address, BreakpointTrigger trigger, bool enable);
		bool SetPreferredHardwareBreakpoints(HANDLE hThread);
		void SetOpcodeBreakpoint(Breakpoint& breakpoint, bool enable) const;
		static void ClearOpcodeBreakpoint(Breakpoint& breakpoint);
		void SetAllBreakpoints();

		static bool IsHwTrigger(BreakpointTrigger trigger)
		{
			return trigger == BreakpointTrigger::Execute || IsDataTrigger(trigger);
		}

		static bool IsDataTrigger(BreakpointTrigger trigger)
		{
			return trigger == BreakpointTrigger::Write || trigger == BreakpointTrigger::ReadWrite;
		}

		static uint8_t HwTriggerValue(BreakpointTrigger trigger)
		{
			return trigger == BreakpointTrigger::Write ? 1 : trigger == BreakpointTrigger::ReadWrite ? 3 : 0;
		}

	public:
		explicit Debugger(std::shared_ptr<ILogger> logger)
			: _logger(move(logger))
		{
			_instance = this;
		}

		void AddBreakpointHandler(IBreakpointHandler* handler) override;
		void RemoveBreakpointHandler(IBreakpointHandler* handler) override;
		bool SetBreakpoint(void* address, BreakpointTrigger trigger) override;
		bool UnsetBreakpoint(void* address) override;
		bool IsBreakpointSet(void* address, BreakpointTrigger* outTrigger) override;

		void RegisterThread(HANDLE hThread);

		virtual ~Debugger();
	};
}
