#pragma once

#include <string>

namespace TrainerLib
{
	class IMessenger
	{
	public:
		virtual ~IMessenger() {}

		virtual bool IsConnected() = 0;

		virtual bool Connect() = 0;

		virtual void Disconnect() = 0;

		virtual bool GetNextValue(std::string& varName, double& value) = 0;

		virtual bool SendValue(std::string varName, double value) = 0;

		virtual bool NotifyInitializationStarted() = 0;

		virtual bool NotifyInitializationIsComplete() = 0;
	};
}
