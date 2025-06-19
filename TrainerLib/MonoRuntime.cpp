#include <Windows.h>
#include <string>
#include <TlHelp32.h>
#include "MonoDefs.h"
#include "MonoRuntime.h"
#include <ThemidaSDK.h>

namespace TrainerLib
{
	int monoIntegrityCheck = 256;

	bool MonoRuntime::Initialize()
	{
		_initLock.lock();

		if (_initialized) {
			_initLock.unlock();
			return true;
		}

		VM_FAST_START

		auto hModule = GetModuleHandleA("mono.dll");

		if (hModule == nullptr) {
			auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
			if (snapshot != INVALID_HANDLE_VALUE) {
				MODULEENTRY32 moduleEntry;
				moduleEntry.dwSize = sizeof(moduleEntry);

				if (Module32First(snapshot, &moduleEntry)) {
					do {
						if (GetProcAddress(moduleEntry.hModule, "mono_thread_attach")) {
							hModule = moduleEntry.hModule;
							break;
						}

					} while (Module32Next(snapshot, &moduleEntry));

				}

				CloseHandle(snapshot);
			}
		}

		if (hModule == nullptr) {
			_initLock.unlock();
			return false;
		}

		CHECK_PROTECTION(monoIntegrityCheck, 128)

		IMPORT_MONO_FUNC(mono_assembly_foreach);
		IMPORT_MONO_FUNC(mono_assembly_get_image);
		IMPORT_MONO_FUNC(mono_image_get_table_info);
		IMPORT_MONO_FUNC(mono_table_info_get_rows);
		IMPORT_MONO_FUNC(mono_class_get);
		IMPORT_MONO_FUNC(mono_class_get_methods);
		IMPORT_MONO_FUNC(mono_compile_method);
		IMPORT_MONO_FUNC(mono_class_from_name);
		IMPORT_MONO_FUNC(mono_class_get_method_from_name);
		IMPORT_MONO_FUNC(mono_method_get_name);
		IMPORT_MONO_FUNC(mono_image_get_name);
		IMPORT_MONO_FUNC(mono_image_loaded);
		IMPORT_MONO_FUNC(mono_image_get_assembly);
		IMPORT_MONO_FUNC(mono_class_get_name);
		IMPORT_MONO_FUNC(mono_thread_attach);
		IMPORT_MONO_FUNC(mono_get_root_domain);
		IMPORT_MONO_FUNC(mono_method_get_flags);
		IMPORT_MONO_FUNC(mono_thread_detach);
		IMPORT_MONO_FUNC(mono_class_get_flags);

		if (monoIntegrityCheck == 128) {
			CHECK_CODE_INTEGRITY(monoIntegrityCheck, 0)
		}

		_logger->Log(LogLevel::Debug, "Mono: Intialized.");

		_initialized = true;

		_initLock.unlock();

		VM_FAST_END

		return true;
	}

	void* MonoRuntime::CompileMethod(MonoMethod* method, const char* className) const
	{
		try {
			return mono_compile_method(method);
		}
		catch (...) {
			_logger->Log(LogLevel::Error, "Mono: Failed to compile method %s in class %s.", mono_method_get_name(method), className);
			return nullptr;
		}
	}

	void MonoRuntime::CompileMethodsInClass(MonoClass* monoClass) const
	{
		std::string className = mono_class_get_name(monoClass);

		if (className.find('`') != std::string::npos) {
			_logger->Log(LogLevel::Debug, "Mono: Skipping compilation of generic class %s.", className.c_str());
			return;
		}

		void* iter = nullptr;
		MonoMethod* method;

		VM_FAST_START

		while ((method = mono_class_get_methods(monoClass, &iter)) != nullptr) {
			if ((mono_method_get_flags(method, nullptr) & METHOD_ATTRIBUTE_ABSTRACT) != uint32_t(monoIntegrityCheck)) {
				continue;
			}

			CompileMethod(method, className.c_str());
		}

		VM_FAST_END

		_logger->Log(LogLevel::Debug, "Mono: Compiled class %s.", className.c_str());
	}

	void _cdecl MonoRuntime::CompileMethodsInAssemblyStatic(MonoAssembly* assembly, void* userData = nullptr)
	{
		static_cast<MonoRuntime*>(userData)->CompileMethodsInAssembly(assembly);
	}

	void MonoRuntime::CompileMethodsInAssembly(MonoAssembly* assembly) const
	{
		auto image = mono_assembly_get_image(assembly);

		if (image == nullptr) {
			_logger->Log(LogLevel::Error, "Mono: Image of assembly not found.");
			return;
		}

		auto tableInfo = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		auto typeCount = mono_table_info_get_rows(tableInfo);

		for (auto x = 0; x < typeCount; x++) {
			CompileMethodsInClass(mono_class_get(image, MONO_TOKEN_TYPE_DEF | (x + 1)));
		}
	}

	/*bool MonoRuntime::CompileAssemblies()
	{
		auto thread = mono_thread_attach(mono_get_root_domain());

		mono_assembly_foreach(CompileMethodsInAssembly, nullptr);

		mono_thread_detach(thread);

		return true;
	}*/

	MonoAssembly* MonoRuntime::GetAssembly(const char* pszAssembly) const
	{
		auto image = mono_image_loaded(pszAssembly);

		if (image == nullptr) {
			return nullptr;
		}

		return mono_image_get_assembly(image);
	}

	MonoClass* MonoRuntime::GetClass(const char* pszAssembly, const char* pszNamespace, const char* pszClass) const
	{
		auto assembly = GetAssembly(pszAssembly);

		if (assembly == nullptr) {
			return nullptr;
		}

		auto image = mono_assembly_get_image(assembly);

		return mono_class_from_name(image, pszNamespace, pszClass);
	}

	MonoMethod* MonoRuntime::GetMethod(const char* pszAssembly, const char* pszNamespace, const char* pszClass, const char* pszMethod, int nParams) const
	{
		auto monoClass = GetClass(pszAssembly, pszNamespace, pszClass);

		if (monoClass == nullptr) {
			return nullptr;
		}

		return mono_class_get_method_from_name(monoClass, pszMethod, nParams);
	}

	bool MonoRuntime::CompileAssembly(const char* pszAssembly)
	{
		if (!Initialize()) {
			return false;
		}

		auto thread = mono_thread_attach(mono_get_root_domain());

		auto assembly = GetAssembly(pszAssembly);

		if (assembly == nullptr) {
			mono_thread_detach(thread);
			_logger->Log(LogLevel::Error, "Mono: Cannot compile unknown assembly %s.", pszAssembly);
			return false;
		}

		CompileMethodsInAssembly(assembly);

		mono_thread_detach(thread);

		return true;
	}

	bool MonoRuntime::CompileClass(const char* pszAssembly, const char* pszNamespace, const char* pszClass)
	{
		if (!Initialize()) {
			return false;
		}

		auto thread = mono_thread_attach(mono_get_root_domain());

		auto monoClass = GetClass(pszAssembly, pszNamespace, pszClass);

		if (monoClass == nullptr) {
			mono_thread_detach(thread);
			return false;
		}

		CompileMethodsInClass(monoClass);

		mono_thread_detach(thread);

		return true;
	}

	void* MonoRuntime::CompileMethod(const char* pszAssembly, const char* pszNamespace, const char* pszClass, const char* pszMethod, int nParams)
	{
		if (!Initialize()) {
			return nullptr;
		}

		auto thread = mono_thread_attach(mono_get_root_domain());

		auto method = GetMethod(pszAssembly, pszNamespace, pszClass, pszMethod, nParams);

		if (method == nullptr) {
			return nullptr;
		}

		auto ret = CompileMethod(method, pszClass);

		mono_thread_detach(thread);

		return ret;
	}

	bool MonoRuntime::IsLoaded()
	{
		return Initialize();
	}

	bool MonoRuntime::IsAssemblyLoaded(const char* pszAssembly)
	{
		if (!Initialize()) {
			return false;
		}

		auto thread = mono_thread_attach(mono_get_root_domain());
		auto ret = GetAssembly(pszAssembly) != nullptr;
		mono_thread_detach(thread);
		return ret;
	}

	bool MonoRuntime::ClassExists(const char* pszAssembly, const char* pszNamespace, const char* pszClass)
	{
		if (!Initialize()) {
			return false;
		}

		auto thread = mono_thread_attach(mono_get_root_domain());
		auto ret = GetClass(pszAssembly, pszNamespace, pszClass) != nullptr;
		mono_thread_detach(thread);
		return ret;
	}

	bool MonoRuntime::MethodExists(const char* pszAssembly, const char* pszNamespace, const char* pszClass, const char* pszMethod, int nParams)
	{
		if (!Initialize()) {
			return false;
		}

		auto thread = mono_thread_attach(mono_get_root_domain());
		auto ret = GetMethod(pszAssembly, pszNamespace, pszClass, pszMethod, nParams) != nullptr;
		mono_thread_detach(thread);
		return ret;
	}
}
