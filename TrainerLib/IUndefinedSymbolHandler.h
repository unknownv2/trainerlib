#pragma once

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
class IUndefinedSymbolHandler
{
public:
	virtual void* HandleUndefinedSymbol(const char* name) = 0;
};