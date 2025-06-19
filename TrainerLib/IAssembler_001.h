#pragma once

namespace TrainerLib
{
	// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
	class IAssembler_001
	{
	public:
		virtual bool Assemble(const char* script, bool forEnable) = 0;
		virtual void* GetSymbolAddress(const char* name) = 0;
		virtual void SetSymbolAddress(const char* name, void* address) = 0;
		virtual void EnableDataScans() = 0;
		virtual void DisableDataScans() = 0;
	};

	static const char* const IAssembler_001_Version = "IAssembler_001";
}