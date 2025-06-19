#include "kernel.h"
#include <intrin.h>
#include <ThemidaSDK.h>

void GetUnixTime(uint32_t* result)
{
	VM_SLOW_START

	auto kSharedData = reinterpret_cast<KUSER_SHARED_DATA*>(0x7FFE0000);

	KSYSTEM_TIME sysTime;

	do {
		sysTime = kSharedData->SystemTime;
	} while (sysTime.High1Time != sysTime.High2Time);

	auto ticks = (uint64_t(sysTime.High1Time) << 32) | uint64_t(sysTime.LowPart);

	*result = uint32_t(ticks / 10000000 - 11644473600LL);

	VM_SLOW_END
}

void GetUnixTimeUnprotected(uint32_t* result)
{
	auto kSharedData = reinterpret_cast<KUSER_SHARED_DATA*>(0x7FFE0000);

	KSYSTEM_TIME sysTime;

	do {
		sysTime = kSharedData->SystemTime;
	} while (sysTime.High1Time != sysTime.High2Time);

	auto ticks = (uint64_t(sysTime.High1Time) << 32) | uint64_t(sysTime.LowPart);

	*result = uint32_t(ticks / 10000000 - 11644473600LL);
}