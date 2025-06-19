#pragma once

#include <string>
#include "ITrainerArgs_001.h"

namespace TrainerLib
{
	class TrainerArgs : public ITrainerArgs_001
	{
		std::string _flags;
		uint32_t _gameVersion;

	public:
		explicit TrainerArgs(std::string flags, uint32_t gameVersion)
			: _flags(flags), _gameVersion(gameVersion)
		{
			
		}

		bool HasFlag(const char* flag) const
		{
			std::string flagStr = flag;
			return _flags == flag
				|| _flags.find(flagStr + " ") != std::string::npos
				|| _flags.find(" " + flagStr) != std::string::npos;
		}

		uint32_t GetGameVersion() override
		{
			return _gameVersion;
		}

		const char* GetFlags() override
		{
			return _flags.c_str();
		}

		bool HasFlags() const
		{
			return !_flags.empty();
		}
	};
}
