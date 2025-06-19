#pragma once

#include <memory>
#include "IUnrealEngine_001.h"
#include "UnrealEventHooker.h"

namespace TrainerLib
{
	class UnrealEngine : public IUnrealEngine_001
	{
		static bool Initialize();

		std::shared_ptr<UnrealEventHooker> _hooker;

	public:
		explicit UnrealEngine(std::shared_ptr<UnrealEventHooker> hooker) : _hooker(move(hooker)) {}

		bool IsProcessUnreal() override;

		IUnrealEventHook* HookEvent(const wchar_t* objectPattern, const wchar_t* functionPattern, IUnrealEventHandler* handler) override;
	};
}
