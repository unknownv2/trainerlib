#include "MinHooker.h"

namespace TrainerLib
{
	bool MinHooker::Status(MH_STATUS status) const
	{
		switch (status) {
		case MH_OK:
			return true;
		case MH_ERROR_ALREADY_CREATED:
			_logger->Warn("Hook Status: AlreadyCreated");
			return true;
		case MH_ERROR_ENABLED:
			_logger->Warn("Hook Status: AlreadyEnabled");
			return true;
		case MH_ERROR_DISABLED:
			_logger->Warn("Hook Status: AlreadyDisabled");
			return true;
		case MH_ERROR_NOT_CREATED:
			_logger->Error("Hook Status: NotCreated");
			return false;
		case MH_ERROR_NOT_EXECUTABLE:
			_logger->Error("Hook Status: NotExecutable");
			return false;
		case MH_ERROR_UNSUPPORTED_FUNCTION:
			_logger->Error("Hook Status: UnsupportedTarget");
			return false;
		case MH_ERROR_MEMORY_ALLOC:
			_logger->Error("Hook Status: MemoryAllocationFailed");
			return false;
		case MH_ERROR_MEMORY_PROTECT:
			_logger->Error("Hook Status: MemoryProtectionFailed");
			return false;
		default:
			_logger->Error("Hook Status: Unknown");
			return false;
		}
	}

	IHook* MinHooker::CreateHook(void* target, void* detour)
	{
		if (target == nullptr) {
			_logger->Error("Attempted to hook a null address.");
			return nullptr;
		}

		void* original;

		auto status = Status(MH_CreateHook(target, detour, &original));

		if (!status) {
			return nullptr;
		}

		return new Hook(*this, target, detour, original);
	}

	void MinHooker::BeginTransaction()
	{
		_transactionLevel++;
	}

	bool MinHooker::CommitTransaction()
	{
		if (!InTransaction()) {
			return true;
		}

		if (--_transactionLevel == 0) {
			Status(MH_ApplyQueued());
		}
			
		return true;
	}
}
