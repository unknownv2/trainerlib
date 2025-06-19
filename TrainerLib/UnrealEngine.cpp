#include "UnrealEngine.h"

namespace TrainerLib
{
	bool _initialized = false;
	bool _unreal = false;

	bool UnrealEngine::Initialize()
	{
		if (_initialized) {
			return _unreal;
		}

		// hook shit

		// assign vars

		_unreal = true;

		return _unreal;
	}

	bool UnrealEngine::IsProcessUnreal()
	{
		return Initialize();
	}

	IUnrealEventHook* UnrealEngine::HookEvent(const wchar_t* objectPattern, const wchar_t* functionPattern, IUnrealEventHandler* handler)
	{
		return _hooker->HookEvent(objectPattern, functionPattern, handler);
	}
}
