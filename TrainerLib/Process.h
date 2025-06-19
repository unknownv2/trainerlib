#pragma once

#include "IProcess.h"

namespace TrainerLib
{
	class Process : public IProcess
	{
	public:
		void* GetModuleBaseAddress(const wchar_t* module) override;
		uint32_t GetModuleTimestamp(const wchar_t* module = nullptr) override;
		void* ScanProcess(const char* terms, void* startAddress, void* endAddress) override;
		void* ScanModule(const char* terms, const wchar_t* module, void* startAddress) override;
		wchar_t* GetMainModuleBaseName() override;
		bool IsImmersive() override;
	};
}
