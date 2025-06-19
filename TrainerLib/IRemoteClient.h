#pragma once

#include "IRemoteClient_001.h"

namespace TrainerLib
{
	class IRemoteClient : public IRemoteClient_001
	{
	public:
		virtual void RunLoop() = 0;

		virtual bool IsConnected() = 0;

		virtual bool Connect() = 0;

		virtual void Disconnect() = 0;

		virtual ~IRemoteClient() {}
	};
}
