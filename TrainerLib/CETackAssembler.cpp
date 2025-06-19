#include <Windows.h>
#include <Psapi.h>
#include "CETackAssembler.h"
#include <ThemidaSDK.h>
#include "crc32.h"
#include "Util.h"
#include <regex>

namespace TrainerLib
{
	std::regex moduleNameRegex("\"[a-zA-Z0-9\\- _]+?\\.exe\"");

	uint8_t randomNumber = RANDOM(255);

	int ceTackIntegrity = 4096;

	int64_t mainModuleBaseAddress = 0;

	auto allocSymbolTemplate = R"(
		[ENABLE]
		alloc(%s, %d, %llx)
		registersymbol(%s)

		[DISABLE]
		unregistersymbol(%s)
		dealloc(%s)
	)";

	auto setSymbolTemplate = R"(
		[ENABLE]
		registersymbol(%s)
		0x%llx:
		%s:

		[DISABLE]
		unregistersymbol(%s)
	)";

	__declspec(noinline) void CETackAssembler::ReplaceMainModuleName(std::string& script) const
	{
		char path[1024];

		GetProcessImageFileNameA(GetCurrentProcess(), path, 1024);

		auto pathStr = std::string(path);

		auto filename = pathStr.substr(pathStr.find_last_of("/\\") + 1);

		auto replace = "\"" + filename + "\"";

		script = regex_replace(script, moduleNameRegex, replace);
	}

#ifdef _M_X64
	std::string x64Registers[] = {
		"rax", "rbx", "rcx", "rdx", "r8", "r9"
	};

	__declspec(noinline) void CETackAssembler::FixX64FarReference(std::string& script)
	{
		for (auto& symbol : _registeredSymbols) {
			auto symSearchTerms = "[" + symbol + "]";

			size_t x = 0;

			while ((x = script.find(symSearchTerms, x)) != std::string::npos) {
				// We found a line that dereferences the mod value's symbol.

				// Find the start of the current line.
				auto startOfLineIndex = script.rfind("\r\n", x);

				startOfLineIndex = startOfLineIndex == std::string::npos ? 0 : startOfLineIndex + 2;

				// Find the end of the current line.
				auto endOfLineIndex = script.find("\r\n", x);

				if (endOfLineIndex == std::string::npos) {
					endOfLineIndex = script.length();
				}

				std::string tempRegister;
				auto registerFound = false;

				// Find the first general-purpose register that isn't being used.
				for (auto reg : x64Registers) {
					auto registerIndex = script.find(reg, startOfLineIndex);

					if (registerIndex == std::string::npos || registerIndex >= endOfLineIndex) {
						registerFound = true;
						tempRegister = reg;
						break;
					}
				}

				if (!registerFound) {
					// This should never happen with the number of reigster's we're checking.
					++x;
					continue;
				}

				// At this point, we know the line, the position of the referenced mod, and the reigster to use.
				// We have to replace backwards or else the indexes will be messed up.

				// First, we add the pop after the line.
				script.insert(endOfLineIndex, "\r\npop " + tempRegister);

				// Second, we replace the referenced symbol with a register.
				script.replace(x + 1, symSearchTerms.length() - 2, tempRegister);

				// Third, we add the two lines that push the register and move the address into it.
				script.insert(startOfLineIndex,
					"push " + tempRegister + "\r\n" +
					"mov " + tempRegister + ", " + symbol + "\r\n");

				// This isn't the actual end because we inserted 
				// more characters, but it's close enoguh.
				x = endOfLineIndex;
			}
		}
	}
#endif

	__declspec(noinline) void NormalizeWhitespace(std::string& script)
	{
		auto find = std::string("\n");
		auto replace = std::string("\r\n");

		size_t x = 0;

		while ((x = script.find(find, x)) != std::string::npos) {
			if (x == 0 || script[x - 1] != '\r') {
				script.replace(x, find.length(), replace);
				x += replace.length();
			} else {
				++x;
			}
		}

		auto tab = std::string("\t");
		x = 0;

		while ((x = script.find(tab, x)) != std::string::npos) {
			if (x == 0 || script[x - 1] == '\n') {
				script.erase(x, 1);
			} else {
				++x;
			}
		}
	}

#ifndef PROTECTED
	HMODULE LoadDevelopmentDll()
	{
		HMODULE hModule;

		GetModuleHandleExA(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<const char*>(LoadDevelopmentDll),
			&hModule);

		wchar_t dllPath[MAX_PATH];
		GetModuleFileNameW(hModule, dllPath, MAX_PATH);

		std::wstring dllPathString = dllPath;
		dllPathString = dllPathString.substr(0, dllPathString.find_last_of(L"\\"));

#ifdef _M_X64
		dllPathString += L"\\CETack_x64.dll";
#else
		dllPathString += L"\\CETack_x86.dll";
#endif

		return LoadLibraryW(dllPathString.c_str());
	}
#endif

	bool CETackAssembler::Initialize()
	{
		if (CeAutoAssemble != nullptr && CeGetAddress != nullptr) {
			return true;
		}
		
		_logger->Log(LogLevel::Debug, "Loading assembler into process...");

		auto hModule = LoadLibraryA("CETack.dll");

#ifndef PROTECTED
		if (hModule == nullptr) {
			hModule = LoadDevelopmentDll();
		}
#endif

		if (hModule == nullptr) {
			_logger->Log(LogLevel::Error, "Assembler DLL not found.");
			return false;
		}

		VM_FAST_START

		CHECK_PROTECTION(ceTackIntegrity, 8192)

		STR_ENCRYPT_START

		CeAutoAssemble = TCeAutoAssemble(GetProcAddress(hModule, "AutoAssemble"));
		CeGetAddress = TCeGetAddress(GetProcAddress(hModule, "GetAddress"));

		STR_ENCRYPT_END

		if (CeAutoAssemble == nullptr || CeGetAddress == nullptr) {
			_logger->Log(LogLevel::Error, "Assembler exports could not be found.");
			return false;
		}

		if (ceTackIntegrity == 8192) {
			CHECK_CODE_INTEGRITY(ceTackIntegrity, 0)
		}

		_logger->Log(LogLevel::Debug, "Assembler loaded.");

		VM_FAST_END

		return true;
	}

	CETackAssembler::~CETackAssembler()
	{
		disposed = true;

		auto x = int32_t(_enabledScripts.size());

		while (--x >= 0) {
			for (size_t i = 0; i < _enabledScripts[x].size(); i++) {
				_enabledScripts[x][i] ^= randomNumber;
			}
			CETackAssembler::Assemble(_enabledScripts[x].c_str(), false);
			for (size_t i = 0; i < _enabledScripts[x].size(); i++) {
				_enabledScripts[x][i] = 0;
			}
		}

		_enabledScripts.clear();
	}

	bool CETackAssembler::Assemble(const char* pszScript, bool forEnable)
	{
		auto script = std::string(pszScript);

		NormalizeWhitespace(script);

		ReplaceMainModuleName(script);

#ifdef _M_X64
		if (!ceTackIntegrity) {
			FixX64FarReference(script);
		}
#endif

		if (!Initialize()) {
			return false;
		}

		try {

			auto scriptChecksum = crc32buf(reinterpret_cast<const uint8_t*>(script.c_str()), script.size());

			auto wasEnabled = _enabledScriptChecksums.find(scriptChecksum) != _enabledScriptChecksums.end();

			if (forEnable && wasEnabled) {
				return true;
			}

			if (!forEnable && !wasEnabled) {
				return true;
			}

			VM_FAST_START

			auto result = CeAutoAssemble(GetCurrentProcess(), script.c_str(), forEnable ? 1 : ceTackIntegrity, 0) != FALSE;

			VM_FAST_END

			if (!result) {
				_logger->Log(LogLevel::Warning, "Failed to assemble.");
				return false;
			}

			if (forEnable) {
				auto obfuscated = script;
				for (size_t x = 0; x < script.length(); x++) {
					obfuscated[x] ^= randomNumber;
				}
				_enabledScriptChecksums.insert(scriptChecksum);
				_enabledScripts.push_back(obfuscated);
			} else if (wasEnabled) {
				auto obfuscated = script;
				for (size_t x = 0; x < script.length(); x++) {
					obfuscated[x] ^= randomNumber;
				}
				_enabledScriptChecksums.erase(scriptChecksum);
				auto existing = find(_enabledScripts.begin(), _enabledScripts.end(), obfuscated);
				if (existing != _enabledScripts.end()) {
					_enabledScripts.erase(existing);
				}
			}

			return true;
		} catch (std::exception e) {
			if (!disposed) {
				_logger->Log(LogLevel::Error, e.what());
			}
		} catch (const char* e) {
			if (!disposed) {
				_logger->Log(LogLevel::Error, e);
			}
		} catch (...) {
			if (!disposed) {
				_logger->Log(LogLevel::Error, "Failed to assemble script. Last Win32: 0x%08x", GetLastError());
			}
		}

		return false;
	}

	void* CETackAssembler::GetSymbolAddress(const char* name)
	{
		if (!Initialize()) {
			return nullptr;
		}

		auto address = reinterpret_cast<void*>(CeGetAddress(GetCurrentProcess(), name));

		return address;
	}

	void* CETackAssembler::AllocateSymbol(const char* name, size_t size, const wchar_t*)
	{
		if (mainModuleBaseAddress == 0) {
			MODULEINFO moduleInfo;
			if (!GetModuleInformation(GetCurrentProcess(), GetModuleHandleW(nullptr), &moduleInfo, sizeof(MODULEINFO))) {
				_logger->Log(LogLevel::Error, "Failed to get base address of main module. Last Win32: 0x%08x", GetLastError());
				return nullptr;
			}
			mainModuleBaseAddress = reinterpret_cast<int64_t>(moduleInfo.lpBaseOfDll);
		}

		char script[512];
		sprintf_s(script, 512, allocSymbolTemplate, name, size, mainModuleBaseAddress, name, name, name);

		if (!Assemble(script, true)) {
			_logger->Log(LogLevel::Error, "Failed to allocate %s symbol.", name);
			return nullptr;
		}

		_registeredSymbols.insert(name);

		return GetSymbolAddress(name);
	}

	void CETackAssembler::SetSymbolAddress(const char* name, void* address)
	{
		char script[512];
		sprintf_s(script, 512, setSymbolTemplate, name, uint64_t(address), name, name);

		if (!Assemble(script, true)) {
			_logger->Log(LogLevel::Error, "Failed to set symbol address for %s.", name);
		} else {
			_registeredSymbols.insert(name);
		}
	}
}
