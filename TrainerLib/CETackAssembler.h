#pragma once

#include <Windows.h>
#include <memory>
#include <set>
#include <vector>
#include "IAssembler.h"
#include "ILogger.h"

namespace TrainerLib
{
	class CETackAssembler : public IAssembler
	{
		using TCeAutoAssemble = BOOL(WINAPI*)(HANDLE hProcess, LPCSTR pszScript, BOOL bEnable, DWORD dwFlag);
		using TCeGetAddress = uintptr_t(WINAPI*)(HANDLE hProcess, LPCSTR pszSymbol);

		TCeAutoAssemble CeAutoAssemble = nullptr;
		TCeGetAddress CeGetAddress = nullptr;

		std::shared_ptr<ILogger> _logger;
		std::set<uint32_t> _enabledScriptChecksums;
		std::vector<std::string> _enabledScripts;
		std::set<std::string> _registeredSymbols;

		bool disposed = false;

		bool Initialize();

		void ReplaceMainModuleName(std::string& script) const;

#ifdef _M_X64
		void FixX64FarReference(std::string& script);
#endif

	public:
		explicit CETackAssembler(std::shared_ptr<ILogger> logger)
			: _logger(move(logger))
		{
			
		}

		~CETackAssembler() override;

		bool Assemble(const char* script, bool forEnable) override;

		void* AllocateSymbol(const char* name, size_t size, const wchar_t* nearModule = nullptr) override;

		void* GetSymbolAddress(const char* name) override;

		void SetSymbolAddress(const char* name, void* address) override;

		void PredefineSymbol(const char* name) override
		{
			_registeredSymbols.insert(name);
		}

		void SetUndefinedSymbolHandler(IUndefinedSymbolHandler*) override
		{
			// Not supported by CETack.
		}

		void EnableDataScans() override
		{
			// Not supported by CETack.
		}

		void DisableDataScans() override
		{
			// Not supported by CETack.
		}
	};
}
