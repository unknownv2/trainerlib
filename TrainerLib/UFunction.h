#pragma once

#include "UObjectBase.h"

namespace TrainerLib
{
	struct UFunction : UObjectBase
	{
		uint8_t GetNumParams()
		{
			return GetPropertyAt<uint8_t>(0x96);
		}
	};
}
