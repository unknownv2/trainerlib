#pragma once

namespace TrainerLib
{
	class ITrainerArgs_001
	{
	public:
		virtual const char* GetFlags() = 0;
		virtual uint32_t GetGameVersion() = 0;
	};

	static const char* const ITrainerArgs_001_Version = "ITrainerArgs_001";
}