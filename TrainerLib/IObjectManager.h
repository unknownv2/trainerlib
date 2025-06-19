#pragma once

#include "DataTypes.h"
#include "IInternalModObject.h"

class IObjectManager
{
public:
	virtual ~IObjectManager() {}
	virtual IInternalModObject* FindObjectById(int32 id) = 0;
	virtual int32 GetNextId() = 0;
	virtual void FreeObject(int32 id) = 0;
};
