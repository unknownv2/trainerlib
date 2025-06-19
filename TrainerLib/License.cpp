#include "License.h"
#include "kernel.h"
#include "smbios.h"
#include "sha256.h"
#include "rsa.h"
#include <ThemidaSDK.h>
#include "Infinity.h"

namespace TrainerLib
{
	void HashModule(HMODULE module, SHA256& mainHash)
	{
		VM_FAST_START

		SHA256 hash;

		auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
		hash.Update(dosHeader, sizeof(IMAGE_DOS_HEADER));

		auto pPeHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint8_t*>(module) + dosHeader->e_lfanew);

		auto peHeader = *pPeHeader;
		peHeader.OptionalHeader.ImageBase = 0;
		peHeader.OptionalHeader.SectionAlignment = 0;
		peHeader.OptionalHeader.FileAlignment = 0;
		hash.Update(&peHeader, sizeof(IMAGE_NT_HEADERS));

		auto numSections = peHeader.FileHeader.NumberOfSections;
		peHeader = { 0 };

		auto sections = reinterpret_cast<IMAGE_SECTION_HEADER*>(pPeHeader + 1);

		for (auto x = 0; x < numSections; x++) {
			auto section = sections[x];
			section.SizeOfRawData = 0;
			section.PointerToRawData = 0;
			if (x == 0) {
				section.Characteristics = 0;
			}
			hash.Update(&section, sizeof(IMAGE_SECTION_HEADER));
		}

		uint8_t rawHash[32];
		hash.Finalize(rawHash);

		mainHash.Update(rawHash, 32);

		VM_FAST_END
	}

	void License::Validate(HMODULE module, IRemoteClient* client)
	{
		VM_SLOW_START

		// Make sure memory can be accessed before proceeding.
		*(&AllowedVars + AllowedVarsSize) = 0;

		SHA256 hash;
		hash.Update(&StartTime, 4);
		hash.Update(&EndTime, 4);

		uint8_t machineId[10];
		GetMachineId(machineId);
		hash.Update(machineId, 10);

		if (AllowedVarsSize != 0) {
			HashModule(module, hash);
		}

		VM_SLOW_END

		VM_FAST_START

		RSAPublicKey PublicKey
		{
			0x450ce257,
			{
				0xdba8d699, 0x48f4530b, 0xcf2aafea, 0xc1c9433b, 0x21ad4f12, 0x90780418, 0xd8358495, 0x8c7b53af,
				0x6d5a7038, 0x59eb8b7a, 0x2c236c1b, 0x94f087da, 0x7911aa71, 0xea40490f, 0xc85a81b0, 0x7805ebbc,
				0xb8770a7e, 0xa4e1497d, 0xd9ebfc81, 0x559b4b96, 0xaf7277e3, 0x4895b12a, 0x216bcb86, 0x91f38eb4,
				0xc968e6f1, 0x1d098475, 0xb151c9af, 0xcd2eb0b1, 0x24c86e09, 0x36ceceb2, 0x5ee4751c, 0x341b0746,
				0x35d47587, 0x89f89520, 0xcadc5d4d, 0x9aefd06d, 0x53b123b2, 0x108e0d1a, 0x4a57fd3d, 0xdc03708e,
				0x817b8c03, 0x5269c159, 0x0624d716, 0x89dbe495, 0x24e7f09c, 0x83bc354a, 0x839fbc3c, 0x46530fd6,
				0xb7c56475, 0xac3da313, 0xb0aa29c4, 0xd80ba671, 0x6ef19039, 0xf8917a30, 0x53e79b67, 0xb1dd6014,
				0x2044198e, 0xbdac18d5, 0x8d945584, 0x2778456c, 0xc85e8a45, 0xbf620edb, 0x266c3aad, 0xb87fb903,
			},
			{
				0xaa6475ef, 0xbc615aee, 0xe8600e8d, 0x9c3b0b25, 0x5ac6909c, 0xa6384038, 0xde4cf941, 0x64088336,
				0xedc64798, 0x929f95fe, 0xeeace3ad, 0xf155324e, 0x5dc7f1ff, 0x0a056d3a, 0x40aa3926, 0x8c0a5e49,
				0xb0ff38d9, 0x4e83bfb5, 0xc763499b, 0x7e6ef518, 0xdbcae9a8, 0x311e22f5, 0x53bacdb0, 0x5cb6ad01,
				0xe6e32b12, 0x3a55653c, 0xcfc61cf0, 0xe1709a74, 0xe989a0d0, 0x367bbb01, 0xa6ac5973, 0x2a827f29,
				0x4736c25c, 0xc1ee0c91, 0x059a7f0b, 0x470830d5, 0x07c6c06e, 0x1dd7af8a, 0x0f04f37c, 0x97d5a978,
				0xbc61c50a, 0xfe572f4b, 0x7bcfd108, 0x7f9a177d, 0x8979e670, 0x4399327f, 0x0ba34791, 0xcd22e81a,
				0xaa23afa9, 0x672c242a, 0x5678d5b3, 0x94fe43ca, 0x0d5d811d, 0x33d585d1, 0x2398e97a, 0x0fe2f624,
				0xc829ba51, 0xeb202a76, 0x25c89119, 0xee48fdc3, 0x975b4e4b, 0x04424e48, 0x7e43f3bb, 0x5b470e9f,
			}
		};

		hash.Update(&AllowedVarsSize, 4);

		VM_FAST_END

		VM_SLOW_START

		std::set<uint32_t> allowedVars;
		auto pAllowedVars = &AllowedVars;

		uint8_t rawHash[32];
		auto result = false;
		uint32_t x;

		for (x = 0; x < AllowedVarsSize; x++) {
			hash.Update(&pAllowedVars[x], 4);

			allowedVars.insert(pAllowedVars[x] ^ Infinity::RandomValue);

			auto backupHash = hash;
			hash.Finalize(rawHash);

			PublicKey.Verify(Signature, rawHash, result);

			if (result) {
				break;
			}

			hash = backupHash;
		}

		if (AllowedVarsSize == 0) {
			allowedVars.insert(~Infinity::RandomValue);
			hash.Finalize(rawHash);
		}

		uint32_t currentTime;
		GetUnixTimeUnprotected(&currentTime);

		if ((x < AllowedVarsSize || AllowedVarsSize == 0) && currentTime >= StartTime && currentTime < EndTime) {

			PublicKey.Verify(Signature, rawHash, result);

			if (result && x + 1 == allowedVars.size()) {

				client->SetAllowedVars(allowedVars);
			}
		}

		VM_SLOW_END
	}
}
