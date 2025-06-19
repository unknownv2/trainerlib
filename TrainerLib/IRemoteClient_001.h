#pragma once

namespace TrainerLib
{
	class IValueChangedHandler
	{
	public:
		virtual void HandleValueChanged(const char* name, double value) = 0;
	};

	class IRemoteClient_001
	{
	public:
		virtual double GetValue(const char* name) = 0;
		virtual void SetValue(const char* name, double value) = 0;
		virtual void AddValueChangedHandler(IValueChangedHandler* handler) = 0;
		virtual void RemoveValueChangedHandler(IValueChangedHandler* handler) = 0;
	};

	static const char* const IRemoteClient_001_Version = "IRemoteClient_001";
}