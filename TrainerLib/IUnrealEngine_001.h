#pragma once

#include <stdint.h>

namespace TrainerLib
{
	class IUnrealEvent
	{
	public:
		virtual void* GetObject() = 0;
		virtual const wchar_t* GetObjectName() = 0;
		virtual const wchar_t* GetFunctionName() = 0;
		virtual void** GetArguments() = 0;
		virtual uint8_t GetArgumentsLength() = 0;
		virtual void* CallOriginal() = 0;
	};

	class IUnrealEventHook
	{
	public:
		virtual void Enable() = 0;
		virtual void Disable() = 0;
		virtual bool Enabled() = 0;
		//virtual ~IUnrealEventHook() = 0;
	};

	class IUnrealEventHandler
	{
	public:
		virtual void* Handle(IUnrealEvent* e) = 0;
		virtual ~IUnrealEventHandler() = 0;
	};

	class IUnrealEngine_001
	{
	public:
		virtual bool IsProcessUnreal() = 0;
		virtual IUnrealEventHook* HookEvent(const wchar_t* objectPattern, const wchar_t* functionPattern, IUnrealEventHandler* handler) = 0;
	};

	static const char* const IUnrealEngine_001_Version = "IUnrealEngine_001";
}
