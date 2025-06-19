#include "UnrealEventHooker.h"
#include "UFunction.h"

namespace TrainerLib
{
	UnrealEventHooker* _instance = nullptr;

	bool UnrealEventHooker::UnrealEventHook::Handles(std::wstring& objectName, std::wstring& functionName) const
	{
		return _enabled && _objectPattern.Matches(objectName) && _functionPattern.Matches(functionName);
	}

	/*UnrealEventHooker::UnrealEventHook::~UnrealEventHook()
	{
		_enabled = false;

		auto iter = find(_hooker._hooks.begin(), _hooker._hooks.end(), this);

		if (iter != _hooker._hooks.end()) {

			std::swap(*iter, _hooker._hooks.back());

			_hooker._hooks.pop_back();
		}

		delete this->Handler;
	}*/

	void* UnrealEventHooker::ProcessEventStatic(UObjectBase* uObject, UFunction* function, void** params)
	{
		auto instance = _instance;

		if (instance == nullptr || uObject == nullptr || function == nullptr) {
			return UProcessEvent(uObject, function, params);
		}

		return instance->ProcessEvent(uObject, function, params);
	}

	void* UnrealEventHooker::ProcessEvent(UObjectBase* object, UFunction* function, void** params)
	{
		UnrealEvent e(object, function, params);

		std::wstring objectName, functionName;

		if (!object->GetFullName(objectName) || !function->GetFullName(functionName)) {
			return e.CallOriginal();
		}

		e.ObjectName = objectName.c_str();
		e.FunctionName = functionName.c_str();
		e.ArgsLength = function->GetNumParams();

		auto resultSet = false;
		void* result = nullptr;

		for (auto& hook : _hooks) {
			if (hook->Handles(objectName, functionName)) {
				result = hook->Handler->Handle(&e);
				resultSet = true;
			}
		}

		if (!resultSet) {
			result = e.CallOriginal();
		}

		return result;
	}

	IUnrealEventHook* UnrealEventHooker::HookEvent(const wchar_t* objectPattern, const wchar_t* functionPattern, IUnrealEventHandler* handler)
	{
		auto hook = new UnrealEventHook(*this, objectPattern, functionPattern, handler);

		_hooks.push_back(hook);

		return hook;
	}
}
