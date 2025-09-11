
#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"
#include "sys_esl.h"
#include "sys_esl_keyvalues.h"

ESL_ENUM(EKVPairType);

EQSCRIPT_TYPE_BEGIN(KVPairValue)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_VAR(type)

	EQSCRIPT_BIND_FUNC(GetString)
	EQSCRIPT_BIND_FUNC(SetString)

	EQSCRIPT_BIND_FUNC(GetInt)
	EQSCRIPT_BIND_FUNC(SetInt)

	EQSCRIPT_BIND_FUNC(GetBool)
	EQSCRIPT_BIND_FUNC(SetBool)

	EQSCRIPT_BIND_FUNC(GetFloat)
	EQSCRIPT_BIND_FUNC(SetFloat)

	EQSCRIPT_BIND_FUNC(SetFrom)

	EQSCRIPT_BIND_FUNC(SetStringValue)
	EQSCRIPT_BIND_FUNC(SetFromString)
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(KVSection)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_FUNC(Clear)
	EQSCRIPT_BIND_FUNC(ClearValues)

	EQSCRIPT_BIND_FUNC(SetName)
	EQSCRIPT_BIND_FUNC(GetName)

	EQSCRIPT_BIND_FUNC(CreateSection)
	EQSCRIPT_BIND_FUNC(AddSection, ESL_APPLY_TRAITS(void, esl::ToCpp<KVSection*>))
	EQSCRIPT_BIND_FUNC(RemoveSectionByName)
	EQSCRIPT_BIND_FUNC(RemoveSection, ESL_APPLY_TRAITS(void, esl::ToCpp<KVSection*>))

	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetKeyString", SetKey, KVSection&, (const char*, const char*))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetKeyInt", SetKey, KVSection&, (const char*, int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetKeyFloat", SetKey, KVSection&, (const char*, float))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetKeyBool", SetKey, KVSection&, (const char*, bool))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetKeySection", SetKey, KVSection&, (const char*, KVSection*), ESL_APPLY_TRAITS(KVSection&, const char*, esl::ToCpp<KVSection*>))

	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddKeyString", AddKey, KVSection&, (const char*, const char*))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddKeyInt", AddKey, KVSection&, (const char*, int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddKeyFloat", AddKey, KVSection&, (const char*, float))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddKeyBool", AddKey, KVSection&, (const char*, bool))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddKeySection", AddKey, KVSection&, (const char*, KVSection*), ESL_APPLY_TRAITS(KVSection&, const char*, esl::ToCpp<KVSection*>))

	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddValueString", AddValue, void, (const char*))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddValueInt", AddValue, void, (int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddValueFloat", AddValue, void, (float))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddValueBool", AddValue, void, (bool))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddValueSection", AddValue, void, (KVSection*), ESL_APPLY_TRAITS(void, esl::ToCpp<KVSection*>))
	//EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddValuePair", AddValue, void, (KVPairValue*))

	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddUniqueValueString", AddUniqueValue, void, (const char*))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddUniqueValueInt", AddUniqueValue, void, (int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddUniqueValueFloat", AddUniqueValue, void, (float))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("AddUniqueValueBool", AddUniqueValue, void, (bool))

	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetValueStringAt", SetValue, void, (const char*, int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetValueIntAt", SetValue, void, (int, int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetValueFloatAt", SetValue, void, (float, int))
	EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetValueBoolAt", SetValue, void, (bool, int))
	//EQSCRIPT_BIND_FUNC_NAMED_OVERLOAD("SetValuePairAt", SetValue, void, KVPairValue*, int)

	EQSCRIPT_BIND_FUNC(MergeFrom)
	EQSCRIPT_BIND_FUNC(Clone)
	EQSCRIPT_BIND_FUNC(CopyTo)

	EQSCRIPT_BIND_FUNC(FindSection)
	EQSCRIPT_BIND_FUNC(Get)

	EQSCRIPT_BIND_FUNC(IsSection)
	EQSCRIPT_BIND_FUNC(IsArray)
	EQSCRIPT_BIND_FUNC(IsDefinition)

	EQSCRIPT_BIND_FUNC(KeyCount)
	EQSCRIPT_BIND_FUNC(KeyAt)
	EQSCRIPT_BIND_FUNC(ValueCount)
	EQSCRIPT_BIND_FUNC(ValueAt)

	EQSCRIPT_BIND_FUNC(GetType)
	EQSCRIPT_BIND_FUNC(SetType)
EQSCRIPT_TYPE_END

// This class only left as lua wrapper
class LuaKeyValues
{
public:
	void Reset()
	{
		m_root.Clear();
	}

	bool LoadFile(const char* pszFileName, int nSearchFlags)
	{
		return KV_LoadFromFile(pszFileName, nSearchFlags, m_root);
	}

	bool SaveFile(const char* pszFileName, int nSearchFlags)
	{
		IFileStreamPtr pStream = g_fileSystem->Open(pszFileName, FS_OPEN_WRITE, nSearchFlags);

		if (pStream)
		{
			KeyValues::WriteText(pStream, m_root, 0, true);
		}
		else
		{
			MsgError("Cannot save keyvalues to file '%s'!\n", pszFileName);
			return false;
		}
		return true;
	}

	KVSection& GetRoot() { return m_root; }
private:
	KVSection				m_root;
};

EQSCRIPT_TYPE_BEGIN(LuaKeyValues)
	EQSCRIPT_BIND_CONSTRUCTOR()
	EQSCRIPT_BIND_FUNC(LoadFile)
	EQSCRIPT_BIND_FUNC(SaveFile)
	EQSCRIPT_BIND_FUNC(GetRoot)
	EQSCRIPT_BIND_FUNC(Reset)
EQSCRIPT_TYPE_END

static void KV_PrintSection(const KVSection& base)
{
	CMemoryStream stream(PP_SL);
	stream.Open(FS_OPEN_WRITE, nullptr, 2048);
	KeyValues::WriteText(&stream, base, 0, true);

	char nullChar = '\0';
	stream.Write(&nullChar, 1, 1);

	Msg("%s\n", stream.GetBasePointer());
}

bool eslSysKeyValuesInit(const esl::ScriptState& state)
{
	LUA_SET_GLOBAL_CONST(state, KVPAIR_STRING);
	LUA_SET_GLOBAL_CONST(state, KVPAIR_INT);
	LUA_SET_GLOBAL_CONST(state, KVPAIR_FLOAT);
	LUA_SET_GLOBAL_CONST(state, KVPAIR_BOOL);
	LUA_SET_GLOBAL_CONST(state, KVPAIR_SECTION);

	LUA_SET_GLOBAL_CONST(state, KV_FLAG_SECTION);
	LUA_SET_GLOBAL_CONST(state, KV_FLAG_NOVALUE);
	LUA_SET_GLOBAL_CONST(state, KV_FLAG_ARRAY);

	// keyvalues related to FS
	state.RegisterClass<KVPairValue>();
	state.RegisterClass<KVSection>();
	state.RegisterClass<LuaKeyValues>();

	state.SetGlobal("KV_GetValueString", EQSCRIPT_CFUNC(KV_GetValueString));
	state.SetGlobal("KV_GetValueInt", EQSCRIPT_CFUNC(KV_GetValueInt));
	state.SetGlobal("KV_GetValueFloat", EQSCRIPT_CFUNC(KV_GetValueFloat));
	state.SetGlobal("KV_GetValueBool", EQSCRIPT_CFUNC(KV_GetValueBool));
	state.SetGlobal("KV_GetVector2D", EQSCRIPT_CFUNC(KV_GetVector2D));
	state.SetGlobal("KV_GetVector3D", EQSCRIPT_CFUNC(KV_GetVector3D));
	state.SetGlobal("KV_GetVector4D", EQSCRIPT_CFUNC(KV_GetVector4D));
	state.SetGlobal("KV_PrintSection", EQSCRIPT_CFUNC(KV_PrintSection));

	return true;
}