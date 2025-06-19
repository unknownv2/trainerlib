#pragma once

#include "IAssembler_001.h"
#include "IUndefinedSymbolHandler.h"

namespace TrainerLib
{
	class IAssembler : public IAssembler_001
	{
	public:
		virtual void PredefineSymbol(const char* name) = 0;
		virtual void* AllocateSymbol(const char* name, size_t size, const wchar_t* nearModule = nullptr) = 0;
		virtual void SetUndefinedSymbolHandler(IUndefinedSymbolHandler* handler) = 0;
		virtual ~IAssembler() {}
	};
}
