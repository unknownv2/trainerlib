#pragma once

#include "DataTypes.h"

class IInternalModObject
{
public:
	virtual ~IInternalModObject() {}
	virtual void* GetState(uint32& length) = 0;
	virtual void Signal(void* inBuffer, uint32 inLength) = 0;
};