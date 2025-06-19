#pragma once

#include "IHooker.h"
#include "StringPattern.h"
#include "UObjectBase.h"
#include "UFunction.h"
#include "IUnrealEngine_001.h"
#include <vector>

namespace TrainerLib
{
	using TProcessEvent = void*(*)(UObjectBase*, UObjectBase*, void**);

	IHook* ProcessEventHook = nullptr;
	TProcessEvent UProcessEvent = nullptr;

	class IUnrealEventHandler;

	class UnrealEventHooker
	{
		struct UnrealEvent : IUnrealEvent
		{
			UObjectBase* Object;
			UObjectBase* Function;
			const wchar_t* ObjectName = nullptr;
			const wchar_t* FunctionName = nullptr;
			void** Args = nullptr;
			uint8_t ArgsLength = 0;

			UnrealEvent(UObjectBase* object, UObjectBase* function, void** args)
				: Object(object), Function(function), Args(args) {}

			void* GetObject() override { return Object; }
			const wchar_t* GetObjectName() override { return ObjectName; }
			const wchar_t* GetFunctionName() override { return FunctionName; }
			void** GetArguments() override { return Args; }
			uint8_t GetArgumentsLength() override { return ArgsLength; }

			void* CallOriginal() override
			{
				auto procEvent = UProcessEvent;

				return procEvent != nullptr ? procEvent(Object, Function, Args) : nullptr;
			}
		};

		class UnrealEventHook : public IUnrealEventHook
		{
			StringPattern _objectPattern;
			StringPattern _functionPattern;
			bool _enabled = false;

			UnrealEventHooker& _hooker;

		public:
			IUnrealEventHandler* Handler;

			UnrealEventHook(UnrealEventHooker& hooker, std::wstring objectPattern, std::wstring functionPattern, IUnrealEventHandler* handler)
				: _objectPattern(objectPattern), _functionPattern(functionPattern), _hooker(hooker), Handler(handler) {}

			void Enable() override
			{
				_enabled = true;
			}

			void Disable() override
			{
				_enabled = false;
			}

			bool Enabled() override
			{
				return _enabled;
			}

			bool Handles(std::wstring& objectName, std::wstring& functionName) const;

			//~UnrealEventHook() override;
		};

		static void* ProcessEventStatic(UObjectBase* uObject, UFunction* function, void** params);

		std::vector<UnrealEventHook*> _hooks;

		void* ProcessEvent(UObjectBase* object, UFunction* function, void** params);

	public:

		IUnrealEventHook* HookEvent(const wchar_t* objectPattern, const wchar_t* functionPattern, IUnrealEventHandler* handler);
	};
}
