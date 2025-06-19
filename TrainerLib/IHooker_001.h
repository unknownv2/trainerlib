#pragma once

namespace TrainerLib
{
	class IHook
	{
	public:
		virtual void* TargetAddress() = 0;
		virtual void* DetourAddress() = 0;
		virtual void* OriginalAddress() = 0;
		virtual bool Enable() = 0;
		virtual bool Disable() = 0;
		virtual void Remove() = 0;
	};

	class IHooker_001
	{
	public:
		virtual IHook* CreateHook(void* target, void* detour) = 0;
		virtual void BeginTransaction() = 0;
		virtual bool CommitTransaction() = 0;
	};

	static const char* const IHooker_001_Version = "IHooker_001";
}