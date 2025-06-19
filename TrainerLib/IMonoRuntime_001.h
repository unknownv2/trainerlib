#pragma once

namespace TrainerLib
{
	class IMonoRuntime_001
	{
	public:
		// Basic checks.
		virtual bool IsLoaded() = 0;
		virtual bool IsAssemblyLoaded(const char* name) = 0;
		virtual bool ClassExists(const char* assembly, const char* name_space, const char* klass) = 0;
		virtual bool MethodExists(const char* assembly, const char* name_space, const char* klass, const char* method, int numParams) = 0;

		// Compiling methods.
		virtual bool CompileAssembly(const char* name) = 0;
		virtual bool CompileClass(const char* assembly, const char* name_space, const char* klass) = 0;
		virtual void* CompileMethod(const char* assembly, const char* name_space, const char* klass, const char* method, int numParams) = 0;
	};

	static const char* const IMonoRuntime_001_Version = "IMonoRuntime_001";
}