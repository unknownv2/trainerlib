#pragma once

#include <Windows.h>
#include <stdint.h>

struct KSYSTEM_TIME
{
	ULONG LowPart;
	LONG High1Time;
	LONG High2Time;
};

struct KUSER_SHARED_DATA
{
	ULONG TickCountLowDeprecated;
	ULONG TickCountMultiplier;
	KSYSTEM_TIME InterruptTime;
	KSYSTEM_TIME SystemTime;
	KSYSTEM_TIME TimeZoneBias;
};

void GetUnixTimeUnprotected(uint32_t* result);
void GetUnixTime(uint32_t* result);