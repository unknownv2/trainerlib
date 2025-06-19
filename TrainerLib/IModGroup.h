#pragma once

#include "IModItem.h"
#include "IModFactory.h"
#include <vector>

enum class GroupLifetime
{ 
	Indefinite,
	OneAccess
};

class IInlineModGroup : IModItem
{
	
};

class IModGroup : public IModItem
{
public:
	virtual ~IModGroup() {}
	virtual GroupLifetime GetLifetimeType() = 0;
	virtual bool CanBeEnabledByUser() = 0;
	virtual vector<Ptr> GetChildren(IModFactory& modFactory) = 0;
};
