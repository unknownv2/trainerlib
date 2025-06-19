#pragma once

#include <Windows.h>
#include "IRemoteClient.h"

namespace TrainerLib
{
	#pragma pack(push) 
	#pragma pack(1)
	// ReSharper disable once CppClassNeedsConstructorBecauseOfUninitializedMember
	class License
	{
		uint8_t Version;
		uint8_t Reserved[15];
		uint8_t Signature[256];
		uint32_t StartTime;
		uint32_t EndTime;
		uint32_t AllowedVarsSize;
		uint32_t AllowedVars; // Checksummed variable names.

	public:
		void Validate(HMODULE module, IRemoteClient* client);
	};
}
