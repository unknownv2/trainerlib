#include "UObjectBase.h"
#include "FString.h"

namespace TrainerLib
{
	using TUObjectGetFullName = void*(*)(UObjectBase*, FString*, void*);

	TUObjectGetFullName UObjectGetFullName = nullptr;

	bool UObjectBase::Initialize()
	{
		if (UObjectGetFullName != nullptr) {
			return true;
		}

		// find UObjectGetFullName

		return UObjectGetFullName != nullptr;
	}

	bool UObjectBase::GetFullName(std::wstring& name)
	{
		FString fString;

		if (UObjectGetFullName(this, &fString, nullptr) == nullptr) {
			return false;
		}

		name = fString.Name;

		return true;
	}
}
