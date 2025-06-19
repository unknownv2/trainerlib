#pragma once

#include <string>

namespace TrainerLib
{
	struct UObjectBase
	{
		static bool Initialize();

		bool GetFullName(std::wstring& name);

		template<typename TProperty>
		TProperty* GetPointerAt(intptr_t offset)
		{
			return reinterpret_cast<TProperty*>(reinterpret_cast<uint8_t*>(this) + offset);
		}

		template<typename TProperty>
		TProperty GetPropertyAt(intptr_t offset)
		{
			return *GetPointerAt<TProperty>(offset);
		}
	};
}
