#pragma once
#include "IHooker.h"
#include <memory>
#include "ILogger.h"

namespace TrainerLib
{
	class Speedhack
	{
		std::shared_ptr<IHooker> _hooker;
		std::shared_ptr<ILogger> _logger;

	public:
		explicit Speedhack(std::shared_ptr<IHooker> hooker, std::shared_ptr<ILogger> logger)
			: _hooker(move(hooker)), _logger(move(logger))
		{
			
		}

		void SetSpeed(double multiplier) const;

		~Speedhack();
	};
}
