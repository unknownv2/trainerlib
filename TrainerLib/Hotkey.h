#pragma once

#include "Key.h"

namespace Interaction
{
	struct Hotkey
	{
		static const Hotkey Empty;

		Key Key1;
		Key Key2;
		Key Key3;
		Key Key4;

		explicit Hotkey(Key key1, Key key2 = Key::None, Key key3 = Key::None, Key key4 = Key::None)
			: Key1(key1), Key2(key2), Key3(key3), Key4(key4) { }

		explicit Hotkey() : Key1(Key::None), Key2(Key::None), Key3(Key::None), Key4(Key::None) { }

		Key operator[] (int index) const
		{
			switch (index) {
			case 0:
				return Key1;
			case 1:
				return Key2;
			case 2:
				return Key3;
			case 3:
				return Key4;
			default:
				return Key::None;
			}
		}

		bool operator== (Hotkey hotkey) const
		{
			return hotkey.Key1 == Key1
				&& hotkey.Key2 == Key2
				&& hotkey.Key3 == Key3
				&& hotkey.Key4 == Key4;
		}

		bool operator!= (Hotkey hotkey) const
		{
			return !operator==(hotkey);
		}

		bool Uses(Key key) const
		{
			return key == Key1 || key == Key2 || key == Key3 || key == Key4;
		}

		bool IsEmpty() const
		{
			return Key1 == Key::None;
		}

		bool IsKeyboardInput() const
		{
			return !IsXInput();
		}

		bool IsXInput() const
		{
			return Key1 >= Key::GamepadLeftTrigger && Key1 <= Key::GamepadY;
		}
	};


}
