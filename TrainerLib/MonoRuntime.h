#pragma once

#include "IMonoRuntime_001.h"
#include "MonoDefs.h"
#include "ILogger.h"
#include <memory>
#include <mutex>

namespace TrainerLib
{
	class MonoRuntime : public IMonoRuntime_001
	{
		#define IMPORT_MONO_FUNC(func)												\
		func = func##_t(GetProcAddress(hModule, "" #func ));						\
		if (func == nullptr) {														\
			_logger->Log(LogLevel::Error, "Failed to get address of " #func ".");	\
			return false;															\
		}

		mono_assembly_foreach_t mono_assembly_foreach = nullptr;
		mono_assembly_get_image_t mono_assembly_get_image = nullptr;
		mono_image_get_table_info_t mono_image_get_table_info = nullptr;
		mono_table_info_get_rows_t mono_table_info_get_rows = nullptr;
		mono_class_get_t mono_class_get = nullptr;
		mono_class_get_methods_t mono_class_get_methods = nullptr;
		mono_compile_method_t mono_compile_method = nullptr;
		mono_class_from_name_t mono_class_from_name = nullptr;
		mono_class_get_method_from_name_t mono_class_get_method_from_name = nullptr;
		mono_method_get_name_t mono_method_get_name = nullptr;
		mono_image_get_name_t mono_image_get_name = nullptr;
		mono_image_loaded_t mono_image_loaded = nullptr;
		mono_image_get_assembly_t mono_image_get_assembly = nullptr;
		mono_class_get_name_t mono_class_get_name = nullptr;
		mono_thread_attach_t mono_thread_attach = nullptr;
		mono_get_root_domain_t mono_get_root_domain = nullptr;
		mono_method_get_flags_t mono_method_get_flags = nullptr;
		mono_thread_detach_t mono_thread_detach = nullptr;
		mono_class_get_flags_t mono_class_get_flags = nullptr;

		std::shared_ptr<ILogger> _logger;
		std::mutex _initLock;
		bool _initialized = false;

		void* CompileMethod(MonoMethod* method, const char* className) const;
		void CompileMethodsInClass(MonoClass* monoClass) const;
		static void _cdecl CompileMethodsInAssemblyStatic(MonoAssembly* assembly, void* userData);
		void CompileMethodsInAssembly(MonoAssembly* assembly) const;
		MonoAssembly* GetAssembly(const char* pszAssembly) const;
		MonoClass* GetClass(const char* pszAssembly, const char* pszNamespace, const char* pszClass) const;
		MonoMethod* GetMethod(const char* pszAssembly, const char* pszNamespace, const char* pszClass, const char* pszMethod, int nParams = -1) const;

	public:
		explicit MonoRuntime(std::shared_ptr<ILogger> logger)
			: _logger(move(logger))
		{
			
		}

		bool Initialize();

		// Basic checks.
		bool IsLoaded() override;
		bool IsAssemblyLoaded(const char* name) override;
		bool ClassExists(const char* assembly, const char* name_space, const char* klass) override;
		bool MethodExists(const char* assembly, const char* name_space, const char* klass, const char* method, int numParams) override;

		// Compiling methods.
		bool CompileAssembly(const char* name) override;
		bool CompileClass(const char* assembly, const char* name_space, const char* klass) override;
		void* CompileMethod(const char* assembly, const char* name_space, const char* klass, const char* method, int numParams) override;
	};
}
