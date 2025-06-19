#pragma once

#include "Hotkey.h"
#include <vector>

namespace Interaction
{
	class IHotkeyListener
	{
	public:
		virtual void SetCallback(void(*callback)(Hotkey)) = 0;

		virtual void SetHotkeys(std::vector<Hotkey> hotkeys) = 0;

		virtual void Start() = 0;

		virtual void Stop() = 0;

		virtual ~IHotkeyListener() { }
	};
}
