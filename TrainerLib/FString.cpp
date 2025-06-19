#include <stdint.h>
#include "FString.h"

namespace TrainerLib
{
	using TFStringFree = uint32_t(*)(wchar_t*);

	TFStringFree FStringFree = nullptr;

	bool FString::Initialize()
	{
		if (FStringFree != nullptr) {
			return true;
		}

		// find FStringFree

		return FStringFree != nullptr;
	}

	FString::~FString()
	{
		auto name = Name;

		if (name != nullptr) {
			FStringFree(name);
		}
	}
}