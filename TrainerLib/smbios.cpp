#include "smbios.h"
#include "sha256.h"
#include <intrin.h>
#include <ThemidaSDK.h>

typedef NTSTATUS(WINAPI *NtQuerySystemInformation)(
	int SystemInformationClass,
	PVOID SystemInformation,
	ULONG SystemInformationLength,
	PULONG ReturnLength);

typedef enum _SYSTEM_FIRMWARE_TABLE_ACTION
{
	SystemFirmwareTable_Enumerate = 0,
	SystemFirmwareTable_Get = 1
} SYSTEM_FIRMWARE_TABLE_ACTION;

struct SYSTEM_FIRMWARE_TABLE_INFORMATION
{
	ULONG ProviderSignature;
	SYSTEM_FIRMWARE_TABLE_ACTION Action;
	ULONG TableID;
	ULONG TableBufferLength;
	UCHAR TableBuffer[1];
};

__declspec(noinline) void ReadSMBIOSInternal(NtQuerySystemInformation ntQuerySystemInformation, RawSMBIOSData*& ptr)
{
	VM_SLOW_START

	SYSTEM_FIRMWARE_TABLE_INFORMATION request;
	request.ProviderSignature = 0x52534D42;
	request.Action = SystemFirmwareTable_Get;
	request.TableID = 0;
	request.TableBufferLength = 0;

	ULONG bufferSize;

	// Get size of SMBIOS table.
	ntQuerySystemInformation(0x004C, &request, sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION), &bufferSize);

	auto buffer = reinterpret_cast<SYSTEM_FIRMWARE_TABLE_INFORMATION*>(new uint8_t[bufferSize]);

	memcpy_s(buffer, bufferSize, &request, sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION));

	ntQuerySystemInformation(0x004C, buffer, bufferSize, &bufferSize);

	ptr = reinterpret_cast<RawSMBIOSData*>(&buffer->TableBuffer);

	VM_SLOW_END
}

#ifndef _AMD64_
void ReadSMBIOS(RawSMBIOSData*& ptr)
{
	VM_SLOW_START
	STR_ENCRYPT_START
		ReadSMBIOSInternal(NtQuerySystemInformation(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation")), ptr);
	STR_ENCRYPT_END
	VM_SLOW_END
}
#else
__declspec(noinline) void ReadSMBIOS(RawSMBIOSData*& ptr)
{
	VM_SLOW_START

	/*uint8_t syscall[] {
	0x4c, 0x8b, 0xd1, // mov r10, rcx
	0xb8, 0, 0, 0, 0, // mov eax, ?
	0x0f, 0x05, // syscall
	0xc3 // retn
	};*/

	uint8_t syscall[11];

	DWORD oldProtect;
	VirtualProtect(syscall, 11, PAGE_EXECUTE_READWRITE, &oldProtect);

	STR_ENCRYPT_START

	syscall[2] = 0xd1;
	syscall[4] = reinterpret_cast<uint8_t*>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation"))[4];
	syscall[1] = 0x8b;
	syscall[6] = 0x00;
	syscall[5] = 0x00;
	syscall[8] = 0x0f;
	syscall[0] = 0x4c;
	syscall[10] = 0xc3;
	syscall[7] = 0x00;
	syscall[3] = 0xb8;
	syscall[9] = 0x05;

	STR_ENCRYPT_END

	ReadSMBIOSInternal(NtQuerySystemInformation(&syscall), ptr);

	for (auto x = 0; x < 11; x++) {
		syscall[x] = 0;
	}

	VirtualProtect(syscall, 11, oldProtect, &oldProtect);

	VM_SLOW_END

	__nop();
}
#endif

__declspec(noinline) void UpdateHash(SHA256& hash, const char* str)
{
	MUTATE_START

	auto currentPartStart = str;
	auto currentPartLen = 0;
	auto appended = false;

	while (*str != '\0') {
		if (*str == ' ') {
			if (currentPartLen > 0) {
				if (appended) {
					currentPartStart--;
					currentPartLen++;
				}
				hash.Update(currentPartStart, currentPartLen);
				appended = true;
				currentPartLen = 0;
				currentPartStart = ++str;
				continue;
			}
			currentPartStart++;
		}
		else {
			currentPartLen++;
		}
		str++;
	}

	if (currentPartLen > 0) {
		if (appended) {
			currentPartStart--;
			currentPartLen++;
		}
		hash.Update(currentPartStart, currentPartLen);
	}

	hash.Update("\0", 1);

	MUTATE_END

	__nop();
}

__declspec(noinline) void ReadMotherboard(RawSMBIOSData* smbios, SHA256& hash)
{
	VM_SLOW_START

	SMBIOSStruct* currentStruct = nullptr;

	while ((currentStruct = smbios->Next(currentStruct)) != nullptr) {
		if (currentStruct->IsType(SMBIOSStructType::BaseboardInformation)) {
			auto baseboard = static_cast<SMBIOSBaseboard*>(currentStruct);
			if (uint8_t(baseboard->GetFeatureFlags()) & uint8_t(SMBIOSBaseboardFeatureFlags::HostingBoard)) {
				UpdateHash(hash, baseboard->GetManufacturer());
				UpdateHash(hash, baseboard->GetProduct());
				UpdateHash(hash, baseboard->GetVersion());
				UpdateHash(hash, baseboard->GetSerialNumber());
			}
		}
	}

	VM_SLOW_END

	__nop();
}

__declspec(noinline) void ReadSystem(RawSMBIOSData* smbios, SHA256& hash)
{
	VM_SLOW_START

	SMBIOSStruct* currentStruct = nullptr;

	while ((currentStruct = smbios->Next(currentStruct)) != nullptr) {
		if (currentStruct->IsType(SMBIOSStructType::SystemInformation)) {
			auto system = static_cast<SMBIOSSystem*>(currentStruct);
			UpdateHash(hash, system->GetManufacturer());
			UpdateHash(hash, system->GetProductName());
			auto uuid = system->GetUuid();
			hash.Update(&uuid, sizeof(UUID));

			// Not available in WMI.
			//UpdateHash(system->GetVersion());
			//UpdateHash(system->GetSerialNumber());
		}
	}

	VM_SLOW_END

	__nop();
}

__declspec(noinline) void GetMachineId(uint8_t buffer[10])
{
	VM_SLOW_START

	RawSMBIOSData* smbios;

	ReadSMBIOS(smbios);

	SHA256 hash;

	ReadMotherboard(smbios, hash);
	ReadSystem(smbios, hash);

	uint8_t sha256[32];
	hash.Finalize(sha256);

	delete (reinterpret_cast<uint8_t*>(smbios) - 16);

	for (auto x = 0; x < 10; x++) {
		buffer[x] = sha256[x];
		sha256[x] = 0;
	}

	VM_SLOW_END
}

__declspec(noinline) SMBIOSStruct* SMBIOSStruct::Next()
{
	MUTATE_START

	auto raw = reinterpret_cast<char*>(this);

	// Skip formatted section.
	raw += GetLength();

	char* ret;

	// Skip unformatted section.
	for (;;) {
		auto cur = *raw;
		++raw;
		if (cur == 0 && *raw == 0) {
			ret = ++raw;
			break;
		}
	}

	auto biosStruct = reinterpret_cast<SMBIOSStruct*>(ret);

	MUTATE_END

	return biosStruct;
}
