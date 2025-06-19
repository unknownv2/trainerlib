#pragma once

#include <cstdint>
#include <Windows.h>

namespace TrainerLib
{
	enum class BreakpointTrigger : int8_t
	{
		None = -1,
		Execute = 0,
		Write = 1,
		ReadWrite = 2,
		Opcode = 3,
		Dereference = 4,
	};

	class IBreakpointHandler
	{
	public:
		virtual bool HandleBreakpoint(void* address, uint32_t threadId, PCONTEXT context, bool post) = 0;
	};

	class IDebugger_001
	{
	public:
		virtual void AddBreakpointHandler(IBreakpointHandler* handler) = 0;
		virtual void RemoveBreakpointHandler(IBreakpointHandler* handler) = 0;
		virtual bool SetBreakpoint(void* address, BreakpointTrigger trigger = BreakpointTrigger::Execute) = 0;
		virtual bool UnsetBreakpoint(void* address) = 0;
		virtual bool IsBreakpointSet(void* address, BreakpointTrigger* trigger = nullptr) = 0;
	};

	static const char* const IDebugger_001_Version = "IDebugger_001";
}
