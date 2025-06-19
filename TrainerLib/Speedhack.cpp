#include <Windows.h>
#include "Speedhack.h"
#include <mutex>

namespace TrainerLib
{
	double currentMultiplier = 1;

	typedef DWORD(WINAPI *GetTickCountT)();
	typedef ULONGLONG(WINAPI *GetTickCount64T)();
	typedef BOOL(WINAPI *QueryPerformanceCounterT)(LARGE_INTEGER*);

	IHook* getTickCount = nullptr;
	IHook* getTickCount64 = nullptr;
	IHook* queryPerformanceCounter = nullptr;

	DWORD initialRealTickCount = 0;
	DWORD initialFakeTickCount = 0;
	ULONGLONG initialRealTickCount64 = 0;
	ULONGLONG initialFakeTickCount64 = 0;
	LARGE_INTEGER initialRealPerformanceCounter = { 0 };
	LARGE_INTEGER initialFakePerformanceCounter { 0 };

	std::mutex getTickCountLock;
	std::mutex getTickCount64Lock;
	std::mutex queryPerformanceCounterLock;

	DWORD GetFakeTickCount()
	{
		auto realTickCount = GetTickCountT(getTickCount->OriginalAddress())();

		auto timePassed = (realTickCount - initialRealTickCount) * currentMultiplier;

		return initialFakeTickCount + DWORD(timePassed);
	}

	DWORD WINAPI GetTickCountDetour()
	{
		getTickCountLock.lock();

		auto result = GetFakeTickCount();

		getTickCountLock.unlock();

		return result;
	}

	ULONGLONG GetFakeTickCount64()
	{
		auto realTickCount = GetTickCount64T(getTickCount64->OriginalAddress())();

		auto timePassed = (realTickCount - initialRealTickCount64) * currentMultiplier;

		return initialFakeTickCount64 + ULONGLONG(timePassed);
	}

	ULONGLONG WINAPI GetTickCount64Detour()
	{
		getTickCount64Lock.lock();

		auto result = GetFakeTickCount64();

		getTickCount64Lock.unlock();

		return result;
	}

	BOOL QueryFakePerformanceCounter(LARGE_INTEGER *lpPerformanceCount)
	{
		if (lpPerformanceCount == nullptr) {
			return FALSE;
		}

		LARGE_INTEGER realCounter;

		auto result = QueryPerformanceCounterT(queryPerformanceCounter->OriginalAddress())(&realCounter);

		if (result == FALSE) {
			return result;
		}

		auto timePassed = (realCounter.QuadPart - initialRealPerformanceCounter.QuadPart) * currentMultiplier;

		lpPerformanceCount->QuadPart = initialFakePerformanceCounter.QuadPart + ULONGLONG(timePassed);

		return result;
	}

	BOOL WINAPI QueryPerformanceCounterDetour(LARGE_INTEGER *lpPerformanceCount)
	{
		queryPerformanceCounterLock.lock();

		auto result = QueryFakePerformanceCounter(lpPerformanceCount);

		queryPerformanceCounterLock.unlock();

		return result;
	}

	void Speedhack::SetSpeed(double multiplier) const
	{
		getTickCountLock.lock();
		getTickCount64Lock.lock();
		queryPerformanceCounterLock.lock();
		
		initialFakeTickCount = getTickCount ? GetFakeTickCount() : GetTickCount();
		initialFakeTickCount64 = getTickCount64 ? GetFakeTickCount64() : GetTickCount64();
		if (queryPerformanceCounter) {
			QueryFakePerformanceCounter(&initialFakePerformanceCounter);
		} else {
			QueryPerformanceCounter(&initialFakePerformanceCounter);
		}

		initialRealTickCount = getTickCount ? GetTickCountT(getTickCount->OriginalAddress())() : initialFakeTickCount;
		initialRealTickCount64 = getTickCount64 ? GetTickCountT(getTickCount64->OriginalAddress())() : initialFakeTickCount64;
		if (queryPerformanceCounter) {
			QueryPerformanceCounterT(queryPerformanceCounter->OriginalAddress())(&initialRealPerformanceCounter);
		} else {
			initialRealPerformanceCounter = initialFakePerformanceCounter;
		}

		currentMultiplier = multiplier;

		if (!getTickCount || !getTickCount64 || !queryPerformanceCounter) {
			
			auto hModule = GetModuleHandleA("kernel32.dll");

			if (hModule != nullptr) {

				_hooker->BeginTransaction();

				if (!getTickCount) {
					auto address = GetProcAddress(hModule, "GetTickCount");
					if (address != nullptr) {
						getTickCount = _hooker->CreateHook(address, GetTickCountDetour);
						if (getTickCount != nullptr) {
							getTickCount->Enable();
						}
					}
				}

				if (!getTickCount64) {
					auto address = GetProcAddress(hModule, "GetTickCount64");
					if (address != nullptr) {
						getTickCount64 = _hooker->CreateHook(address, GetTickCount64Detour);
						if (getTickCount64 != nullptr) {
							getTickCount64->Enable();
						}
					}
				}

				if (!queryPerformanceCounter) {
					auto address = GetProcAddress(hModule, "QueryPerformanceCounter");
					if (address != nullptr) {
						queryPerformanceCounter = _hooker->CreateHook(address, QueryPerformanceCounterDetour);
						if (queryPerformanceCounter != nullptr) {
							queryPerformanceCounter->Enable();
						}
					}
				}

				_hooker->CommitTransaction();
			}
		}

		getTickCountLock.unlock();
		getTickCount64Lock.unlock();
		queryPerformanceCounterLock.unlock();
	}

	Speedhack::~Speedhack()
	{
		if (!getTickCount && !getTickCount64 && !queryPerformanceCounter) {
			return;
		}

		getTickCountLock.lock();
		getTickCount64Lock.lock();
		queryPerformanceCounterLock.lock();

		_hooker->BeginTransaction();

		if (getTickCount) {
			delete getTickCount;
			getTickCount = nullptr;
		}

		if (getTickCount64) {
			delete getTickCount64;
			getTickCount64 = nullptr;
		}

		if (queryPerformanceCounter) {
			delete queryPerformanceCounter;
			queryPerformanceCounter = nullptr;
		}

		_hooker->CommitTransaction();

		currentMultiplier = 1;

		getTickCountLock.unlock();
		getTickCount64Lock.unlock();
		queryPerformanceCounterLock.unlock();
	}
}
