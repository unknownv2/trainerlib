#pragma once

#include <cstdint>

namespace TrainerLib
{
	class IProcess_001
	{
	public:
		virtual void* GetModuleBaseAddress(const wchar_t* module) = 0;
		virtual uint32_t GetModuleTimestamp(const wchar_t* module = nullptr) = 0;
		virtual void* ScanProcess(const char* terms, void* startAddress, void* endAddress) = 0;
		virtual void* ScanModule(const char* terms, const wchar_t* module, void* startAddress) = 0;
		virtual wchar_t* GetMainModuleBaseName() = 0;
		virtual bool IsImmersive() = 0;
	};

	static const char* const IProcess_001_Version = "IProcess_001";
}
