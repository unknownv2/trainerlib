#include <Windows.h>
#include <Psapi.h>
#include "Process.h"
#include <string>
#include <algorithm>
#include <vector>
#include "Util.h"
#include <ThemidaSDK.h>

namespace TrainerLib
{
	int randomSeed = 100;
	int seedCheck = 100;

	void* _mainModuleBase = nullptr;
	uint32_t _mainModuleSize = 0;

	uint32_t GetModuleSize(const wchar_t* pszModuleName)
	{
		if (pszModuleName == nullptr && _mainModuleSize != 0) {
			return _mainModuleSize;
		}

		auto moduleHandle = GetModuleHandle(pszModuleName);

		if (moduleHandle == nullptr) {
			return 0;
		}

		MODULEINFO moduleInfo;

		if (!GetModuleInformation(
			GetCurrentProcess(),
			moduleHandle,
			&moduleInfo,
			sizeof(MODULEINFO)))
		{
			return 0;
		}

		if (pszModuleName == nullptr) {
			_mainModuleSize = moduleInfo.SizeOfImage;
		}

		return moduleInfo.SizeOfImage;
	}

	void* Process::GetModuleBaseAddress(const wchar_t* pszModuleName)
	{
		if (pszModuleName == nullptr && _mainModuleBase != nullptr) {
			return _mainModuleBase;
		}

		auto moduleHandle = GetModuleHandleW(pszModuleName);

		if (moduleHandle == nullptr) {
			return nullptr;
		}

		MODULEINFO moduleInfo;

		if (!GetModuleInformation(
			GetCurrentProcess(),
			moduleHandle,
			&moduleInfo,
			sizeof(MODULEINFO)))
		{
			return nullptr;
		}

		if (pszModuleName == nullptr) {
			_mainModuleBase = moduleInfo.lpBaseOfDll;
		}

		return moduleInfo.lpBaseOfDll;
	}

	uint32_t Process::GetModuleTimestamp(const wchar_t* module)
	{
		auto baseAddress = GetModuleBaseAddress(module);

		if (baseAddress == nullptr) {
			return 0;
		}

		auto peHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(
			static_cast<uint8_t*>(baseAddress) + 
			static_cast<IMAGE_DOS_HEADER*>(baseAddress)->e_lfanew);

		return peHeader->FileHeader.TimeDateStamp;
	}

	int WildcardHexToInt(char input)
	{
		if (input == '?') {
			return -1;
		}

		if (input >= '0' && input <= '9') {
			return input - '0';
		}

		if (input >= 'A' && input <= 'F') {
			return input - 'A' + 10;
		}

		if (input >= 'a' && input <= 'f') {
			return input - 'a' + 10;
		}

		return -2;
	}

	bool ParseHexString(const char* pszTerms, std::vector<uint8_t>& bytes, std::vector<uint8_t>& wildcardFlags, bool& hasWildcard)
	{
		std::string terms(pszTerms);

		terms.erase(remove_if(terms.begin(), terms.end(), isspace), terms.end());

		if (terms.length() == 0) {
			bytes.resize(0);
			return true;
		}

		if (terms.length() % 2 != 0) {
			terms.append(1, '?');
		}

		auto byteLen = terms.length() / 2;

		bytes.resize(byteLen);
		wildcardFlags.resize(byteLen);

		// Deserialize the hex string.
		for (size_t xChar = 0, xByte = 0; xByte < byteLen; xChar++, xByte++) {
			auto hex = WildcardHexToInt(terms[xChar]);

			if (hex == -2) {
				return false;
			}

			if (hex == -1) {
				wildcardFlags[xByte] = 1;
				hasWildcard = true;
			}
			else {
				bytes[xByte] |= hex << 4;
			}

			++xChar;

			hex = WildcardHexToInt(terms[xChar]);

			if (hex == -2) {
				return false;
			}

			if (hex == -1) {
				wildcardFlags[xByte] |= 2;
				hasWildcard = true;
			}
			else {
				bytes[xByte] |= hex;
			}
		}

		return true;
	}

	void* ScanContiguousRegion(void* lpStartAddress, void* lpEndAddress, const std::vector<uint8_t>& bytes, const std::vector<uint8_t>& wildcardFlags, const bool& hasWildcard)
	{
		if (lpStartAddress >= lpEndAddress) {
			return nullptr;
		}

		auto byteLen = bytes.size();
		auto addr = static_cast<uint8_t*>(lpStartAddress);

		// Use memcmp if there are no wildcards.
		if (!hasWildcard) {
			auto byteArray = &bytes.front();

			do {
				if (memcmp(addr, byteArray, byteLen) == 0 && addr != byteArray) {
					return addr;
				}
			} while (++addr + byteLen <= lpEndAddress);

			return nullptr;
		}

		do {
			size_t x = 0;
			auto tempAddr = addr;

			for (x = 0; x < byteLen; x++, tempAddr++) {
				auto wildcard = wildcardFlags[x];

				if (wildcard == 3) {
					continue;
				}

				if (wildcard == 0) {
					if (*tempAddr != bytes[x]) {
						break;
					}
				}
				else if (wildcard == 1) {
					if ((*tempAddr & 0x0f) != bytes[x]) {
						break;
					}
				}
				else if (wildcard == 2) {
					if ((*tempAddr & 0xf0) != bytes[x]) {
						break;
					}
				}
			}

			if (x == byteLen && addr != &bytes.front()) {
				return addr;
			}
		} while (++addr + byteLen <= lpEndAddress);

		return nullptr;
	}

	void* TryScanContiguousRegion(void* lpStartAddress, void* lpEndAddress, const std::vector<uint8_t>& bytes, const std::vector<uint8_t>& wildcardFlags, const bool& hasWildcard)
	{
		__try {
			return ScanContiguousRegion(lpStartAddress, lpEndAddress, bytes, wildcardFlags, hasWildcard);
		}
		__except (true) {
			// Ignore access violations due to race conditions.
			return nullptr;
		}
	}

	void* Process::ScanProcess(const char* pszTerms, void* lpStartAddress, void* lpEndAddress)
	{
		if (lpEndAddress == nullptr) {
			lpEndAddress = reinterpret_cast<void*>(0x7ffffffffff);
		}

		if (lpStartAddress >= lpEndAddress) {
			return nullptr;
		}

		auto byteLen = strlen(pszTerms) / 2;

		std::vector<uint8_t> bytes(byteLen);
		std::vector<uint8_t> wildcardFlags(byteLen);
		auto hasWildcard = false;

		if (!ParseHexString(pszTerms, bytes, wildcardFlags, hasWildcard)) {
			return nullptr;
		}

		MEMORY_BASIC_INFORMATION mem;

		auto addr = static_cast<uint8_t*>(lpStartAddress);
		void* ret = nullptr;

		while (VirtualQuery(addr, &mem, sizeof mem)) {
			if (mem.BaseAddress >= lpEndAddress) {
				break;
			}

			if ((mem.State & MEM_FREE) == 0 && (mem.Protect & 0x01) == 0 && mem.Protect != 0) {
				if ((ret = TryScanContiguousRegion(addr, min(addr + mem.RegionSize, lpEndAddress), bytes, wildcardFlags, hasWildcard)) != nullptr) {
					break;
				}
			}

			addr = static_cast<uint8_t*>(mem.BaseAddress) + mem.RegionSize;

			if (addr >= lpEndAddress) {
				break;
			}
		}

		return ret;
	}

	void* Process::ScanModule(const char* pszTerms, const wchar_t* pszModuleName, void* lpStartAddress)
	{
		auto baseAddress = GetModuleBaseAddress(pszModuleName);

		if (baseAddress == nullptr) {
			return nullptr;
		}

		if (lpStartAddress == nullptr) {
			lpStartAddress = baseAddress;
		}

		auto moduleSize = GetModuleSize(pszModuleName);

		if (moduleSize == 0) {
			return nullptr;
		}

		auto endAddress = static_cast<uint8_t*>(baseAddress) + moduleSize;

		if (lpStartAddress < baseAddress || lpStartAddress >= endAddress) {
			return nullptr;
		}

		auto byteLen = strlen(pszTerms) / 2;

		std::vector<uint8_t> bytes(byteLen);
		std::vector<uint8_t> wildcardFlags(byteLen);
		bool hasWildcard;

		if (!ParseHexString(pszTerms, bytes, wildcardFlags, hasWildcard)) {
			return nullptr;
		}

		VM_FAST_START

		if (seedCheck == 100) {
			CHECK_PROTECTION(randomSeed, 101)

			if (randomSeed == 101) {
				CHECK_CODE_INTEGRITY(randomSeed, 64);
			}
		}

		auto result = ScanContiguousRegion(lpStartAddress, endAddress, bytes, wildcardFlags, hasWildcard);

		seedCheck = int(uintptr_t(result) % 99);

		if (RANDOM(randomSeed) > 80) {
			result = nullptr;
		}

		VM_FAST_END

		return result;
	}

	wchar_t* Process::GetMainModuleBaseName()
	{
		auto name = new wchar_t[MAX_PATH];

		if (!GetModuleBaseNameW(GetCurrentProcess(), nullptr, name, MAX_PATH)) {
			return nullptr;
		}

		return name;
	}

	bool Process::IsImmersive()
	{
		typedef BOOL(WINAPI* IsImmersiveProcessFunc)(HANDLE process);

		auto user32 = GetModuleHandleA("user32.dll");

		if (user32 == nullptr) {
			return false;
		}

		auto isImmersiveProcess = reinterpret_cast<IsImmersiveProcessFunc>(GetProcAddress(user32, "IsImmersiveProcess"));

		return isImmersiveProcess ? isImmersiveProcess(GetCurrentProcess()) != FALSE : false;
	}
}
