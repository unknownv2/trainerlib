#pragma once

#include <cstdint>

namespace TrainerLib
{
	typedef enum {
		MONO_TOKEN_MODULE = 0x00000000,
		MONO_TOKEN_TYPE_REF = 0x01000000,
		MONO_TOKEN_TYPE_DEF = 0x02000000,
		MONO_TOKEN_FIELD_DEF = 0x04000000,
		MONO_TOKEN_METHOD_DEF = 0x06000000,
		MONO_TOKEN_PARAM_DEF = 0x08000000,
		MONO_TOKEN_INTERFACE_IMPL = 0x09000000,
		MONO_TOKEN_MEMBER_REF = 0x0a000000,
		MONO_TOKEN_CUSTOM_ATTRIBUTE = 0x0c000000,
		MONO_TOKEN_PERMISSION = 0x0e000000,
		MONO_TOKEN_SIGNATURE = 0x11000000,
		MONO_TOKEN_EVENT = 0x14000000,
		MONO_TOKEN_PROPERTY = 0x17000000,
		MONO_TOKEN_MODULE_REF = 0x1a000000,
		MONO_TOKEN_TYPE_SPEC = 0x1b000000,
		MONO_TOKEN_ASSEMBLY = 0x20000000,
		MONO_TOKEN_ASSEMBLY_REF = 0x23000000,
		MONO_TOKEN_FILE = 0x26000000,
		MONO_TOKEN_EXPORTED_TYPE = 0x27000000,
		MONO_TOKEN_MANIFEST_RESOURCE = 0x28000000,
		MONO_TOKEN_GENERIC_PARAM = 0x2a000000,
		MONO_TOKEN_METHOD_SPEC = 0x2b000000,

		MONO_TOKEN_STRING = 0x70000000,
		MONO_TOKEN_NAME = 0x71000000,
		MONO_TOKEN_BASE_TYPE = 0x72000000
	} MonoTokenType;

	typedef enum {
		MONO_TABLE_MODULE,
		MONO_TABLE_TYPEREF,
		MONO_TABLE_TYPEDEF,
		MONO_TABLE_FIELD_POINTER,
		MONO_TABLE_FIELD,
		MONO_TABLE_METHOD_POINTER,
		MONO_TABLE_METHOD,
		MONO_TABLE_PARAM_POINTER,
		MONO_TABLE_PARAM,
		MONO_TABLE_INTERFACEIMPL,
		MONO_TABLE_MEMBERREF, /* 0xa */
		MONO_TABLE_CONSTANT,
		MONO_TABLE_CUSTOMATTRIBUTE,
		MONO_TABLE_FIELDMARSHAL,
		MONO_TABLE_DECLSECURITY,
		MONO_TABLE_CLASSLAYOUT,
		MONO_TABLE_FIELDLAYOUT, /* 0x10 */
		MONO_TABLE_STANDALONESIG,
		MONO_TABLE_EVENTMAP,
		MONO_TABLE_EVENT_POINTER,
		MONO_TABLE_EVENT,
		MONO_TABLE_PROPERTYMAP,
		MONO_TABLE_PROPERTY_POINTER,
		MONO_TABLE_PROPERTY,
		MONO_TABLE_METHODSEMANTICS,
		MONO_TABLE_METHODIMPL,
		MONO_TABLE_MODULEREF, /* 0x1a */
		MONO_TABLE_TYPESPEC,
		MONO_TABLE_IMPLMAP,
		MONO_TABLE_FIELDRVA,
		MONO_TABLE_UNUSED6,
		MONO_TABLE_UNUSED7,
		MONO_TABLE_ASSEMBLY, /* 0x20 */
		MONO_TABLE_ASSEMBLYPROCESSOR,
		MONO_TABLE_ASSEMBLYOS,
		MONO_TABLE_ASSEMBLYREF,
		MONO_TABLE_ASSEMBLYREFPROCESSOR,
		MONO_TABLE_ASSEMBLYREFOS,
		MONO_TABLE_FILE,
		MONO_TABLE_EXPORTEDTYPE,
		MONO_TABLE_MANIFESTRESOURCE,
		MONO_TABLE_NESTEDCLASS,
		MONO_TABLE_GENERICPARAM, /* 0x2a */
		MONO_TABLE_METHODSPEC,
		MONO_TABLE_GENERICPARAMCONSTRAINT,
		MONO_TABLE_UNUSED8,
		MONO_TABLE_UNUSED9,
		MONO_TABLE_UNUSED10,
		/* Portable PDB tables */
		MONO_TABLE_DOCUMENT, /* 0x30 */
		MONO_TABLE_METHODBODY,
		MONO_TABLE_LOCALSCOPE,
		MONO_TABLE_LOCALVARIABLE,
		MONO_TABLE_LOCALCONSTANT,
		MONO_TABLE_IMPORTSCOPE,
		MONO_TABLE_ASYNCMETHOD,
		MONO_TABLE_CUSTOMDEBUGINFORMATION
	} MonoMetaTableEnum;

#define METHOD_ATTRIBUTE_VIRTUAL                   0x0040
#define METHOD_ATTRIBUTE_ABSTRACT                  0x0400

	using MonoDomain = void;
	using MonoImage = void;
	using MonoAssembly = char;
	using MonoAssemblyName = char;
	using MonoClass = void;
	using MonoMethod = void;
	using MonoTableInfo = void;
	using MonoFunc = void(_cdecl*)(MonoAssembly* assembly, void* user_data);

	using mono_assembly_foreach_t = void(*)(MonoFunc func, void* user_data);
	using mono_assembly_get_image_t = MonoImage* (*)(MonoAssembly *assembly);
	using mono_image_get_table_info_t = const MonoTableInfo* (*)(MonoImage* image, int table_id);
	using mono_table_info_get_rows_t = int(*)(const MonoTableInfo* table);
	using mono_class_get_t = MonoClass* (*)(MonoImage* image, uint32_t type_token);
	using mono_class_get_methods_t = MonoMethod* (*)(MonoClass* klass, void** iter);
	using mono_compile_method_t = void* (*)(MonoMethod *method);
	using mono_class_from_name_t = MonoClass* (*)(MonoImage *image, const char* name_space, const char *name);
	using mono_class_get_method_from_name_t = MonoMethod* (*)(MonoClass *klass, const char *name, int param_count);
	using mono_method_get_name_t = const char* (*)(MonoMethod *method);
	using mono_image_get_name_t = const char* (*)(MonoImage *image);
	using mono_image_loaded_t = MonoImage* (*)(const char *name);
	using mono_image_get_assembly_t = MonoAssembly* (*)(MonoImage *image);
	using mono_class_get_name_t = const char* (*)(MonoClass *klass);
	using mono_thread_attach_t = void* (*)(MonoDomain* domain);
	using mono_get_root_domain_t = MonoDomain* (*)(void);
	using mono_method_get_flags_t = uint32_t(*)(MonoMethod *method, uint32_t *iflags);
	using mono_thread_detach_t = void(*)(void* thread);
	using mono_class_get_flags_t = uint32_t(*)(MonoClass *klass);
}