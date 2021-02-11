//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium core bindings
//
//
//
//////////////////////////////////////////////////////////////////////////////////

// TODO

#include "LuaBinding_Engine.h"

#include "core_base_header.h"

#include "Network/NETThread.h"

#include "sys_state.h"
#include "sys_host.h"

//---------------------------------------------------------------------------------------
// Debug output
//---------------------------------------------------------------------------------------

void WMsg(const char* str)
{
	Msg(str);
}

void WMsgInfo(const char* str)
{
	MsgInfo(str);
}

void WMsgWarning(const char* str)
{
	MsgWarning(str);
}

void WMsgError(const char* str)
{
	MsgError(str);
}

void WMsgAccept(const char* str)
{
	MsgAccept(str);
}

OOLUA_CFUNC( WMsg, LMsg )
OOLUA_CFUNC( WMsgInfo, LMsgInfo )
OOLUA_CFUNC( WMsgWarning, LMsgWarning)
OOLUA_CFUNC( WMsgError, LMsgError)
OOLUA_CFUNC( WMsgAccept, LMsgAccept)

void WMsgBox(const char* str)
{
	InfoMsg(str);
}

void WMsgBoxWarning(const char* str)
{
	WarningMsg(str);
}

void WMsgBoxError(const char* str)
{
	ErrorMsg(str);
}

void WMsgBoxAbort(const char* str)
{
	CrashMsg(str);
}

OOLUA_CFUNC( WMsgBox, LMsgBox )
OOLUA_CFUNC( WMsgBoxWarning, LMsgBoxWarning)
OOLUA_CFUNC( WMsgBoxError, LMsgBoxError)
OOLUA_CFUNC( WMsgBoxAbort, LMsgBoxAbort)

void DebugMessages_InitBinding(lua_State* state)
{
	OOLUA::set_global(state, "Msg", LMsg);
	OOLUA::set_global(state, "MsgInfo", LMsgInfo);
	OOLUA::set_global(state, "MsgWarning", LMsgWarning);
	OOLUA::set_global(state, "MsgError", LMsgError);
	OOLUA::set_global(state, "MsgAccept", LMsgAccept);

	OOLUA::set_global(state, "MsgBox", LMsgBox);
	OOLUA::set_global(state, "MsgBoxWarning", LMsgBoxWarning);
	OOLUA::set_global(state, "MsgBoxError", LMsgBoxError);
	OOLUA::set_global(state, "MsgBoxAbort", LMsgBoxAbort);
}

//---------------------------------------------------------------------------------------
// Core
//---------------------------------------------------------------------------------------

kvkeybase_t* S_DkCore_GetConfig(const char* section_name)
{
	KeyValues* kvs = GetCore()->GetConfig();
	return kvs ? kvs->FindKeyBase(section_name) : nullptr;
}

OOLUA_CFUNC(S_DkCore_GetConfig, L_DkCore_GetConfig)

//---------------------------------------------------------------------------------------
// Filesystem
//---------------------------------------------------------------------------------------

std::string S_IVirtualStream_Read(IVirtualStream* fs, size_t count, size_t size)
{
	char* temp = (char*)stackalloc(count*size + 16);
	fs->Read(temp, count, size);

	return temp;
}

size_t S_IVirtualStream_Write(IVirtualStream* fs, std::string data)
{
	return fs->Write(data.c_str(), data.length(), 1);
}

std::string S_IFileSystem_GetFileBuffer(IFileSystem* fs, const char* filename, int searchFlags = -1)
{
	const char* buf = fs->GetFileBuffer(filename, nullptr, searchFlags);
	std::string fileBuf = buf ? buf : "";
	PPFree((char*)buf);
	return fileBuf;
}

bool S_IFileSystem_AddPackage_NoMount(IFileSystem* fs, const char* packageName, SearchPath_e type)
{
	return fs->AddPackage(packageName, type);
}

OOLUA_CFUNC(S_IVirtualStream_Read, L_IVirtualStream_Read)
OOLUA_CFUNC(S_IVirtualStream_Write, L_IVirtualStream_Write)

OOLUA_CFUNC(S_IFileSystem_AddPackage_NoMount, L_IFileSystem_AddPackage_NoMount)
OOLUA_CFUNC(S_IFileSystem_GetFileBuffer, L_IFileSystem_GetFileBuffer)

OOLUA_EXPORT_FUNCTIONS(IFile, Seek, Tell, GetSize, Flush, GetCRC32)
OOLUA_EXPORT_FUNCTIONS_CONST(IFile)

OOLUA_EXPORT_FUNCTIONS(IFileSystem, Open, Close, FileCopy, GetFileSize, GetFileCRC32, AddPackageMnt, RemovePackage, AddSearchPath, RemoveSearchPath)
OOLUA_EXPORT_FUNCTIONS_CONST(IFileSystem, FileExist, FileRemove, GetCurrentGameDirectory, GetCurrentDataDirectory, MakeDir, RemoveDir)

OOLUA_EXPORT_FUNCTIONS(CFileSystemFind, Init, Next)
OOLUA_EXPORT_FUNCTIONS_CONST(CFileSystemFind, IsDirectory, GetPath)

OOLUA_EXPORT_FUNCTIONS(kvpairvalue_t, SetValueFrom, SetStringValue, SetValueFromString)
OOLUA_EXPORT_FUNCTIONS_CONST(kvpairvalue_t, get_type, get_value, get_nValue, get_bValue, get_fValue)

OOLUA_EXPORT_FUNCTIONS(kvkeybase_t,
	Cleanup,
	ClearValues,

	SetName,

	AddKeyBase,
	AddExistingKeyBase,
	RemoveKeyBaseByName,
	RemoveKeyBase,

	SetKeyString,
	SetKeyInt,
	SetKeyFloat,
	SetKeyBool,
	SetKeySection,

	AddKeyString,
	AddKeyInt,
	AddKeyFloat,
	AddKeyBool,
	//AddKeySection,

	AddValueString,
	AddValueInt,
	AddValueFloat,
	AddValueBool,
	//AddValueSection,
	//AddValuePair,

	AddUniqueValueString,
	AddUniqueValueInt,
	AddUniqueValueFloat,
	AddUniqueValueBool,

	SetValueStringAt,
	SetValueIntAt,
	SetValueFloatAt,
	SetValueBoolAt,
	//SetValuePairAt,

	MergeFrom,
	SetType
)

OOLUA_EXPORT_FUNCTIONS_CONST(kvkeybase_t,
	GetName,
	FindKeyBase,
	Clone,
	CopyTo,
	IsSection,
	IsArray,
	IsDefinition,
	KeyCount,
	KeyAt,
	ValueCount,
	ValueAt,
	GetType
)

OOLUA_EXPORT_FUNCTIONS(KeyValues, LoadFile, SaveFile, GetRoot, Reset)
OOLUA_EXPORT_FUNCTIONS_CONST(KeyValues)

// safe and fast value returner
OOLUA_CFUNC(KV_GetValueString, L_KV_GetValueString)
OOLUA_CFUNC(KV_GetValueInt, L_KV_GetValueInt)
OOLUA_CFUNC(KV_GetValueFloat, L_KV_GetValueFloat)
OOLUA_CFUNC(KV_GetValueBool, L_KV_GetValueBool)

OOLUA_CFUNC(KV_GetVector2D, L_KV_GetVector2D)
OOLUA_CFUNC(KV_GetVector3D, L_KV_GetVector3D)
OOLUA_CFUNC(KV_GetVector4D, L_KV_GetVector4D)

OOLUA_CFUNC(KV_PrintSection, L_KV_PrintSection)

void FileSystem_InitBinding(lua_State* state)
{
	LUA_SET_GLOBAL_ENUMCONST(state, KVPAIR_STRING);
	LUA_SET_GLOBAL_ENUMCONST(state, KVPAIR_INT);
	LUA_SET_GLOBAL_ENUMCONST(state, KVPAIR_FLOAT);
	LUA_SET_GLOBAL_ENUMCONST(state, KVPAIR_BOOL);
	LUA_SET_GLOBAL_ENUMCONST(state, KVPAIR_SECTION);

	LUA_SET_GLOBAL_ENUMCONST(state, SP_DATA);
	LUA_SET_GLOBAL_ENUMCONST(state, SP_MOD);
	LUA_SET_GLOBAL_ENUMCONST(state, SP_ROOT);

	LUA_SET_GLOBAL_ENUMCONST(state, KV_FLAG_SECTION);
	LUA_SET_GLOBAL_ENUMCONST(state, KV_FLAG_NOVALUE);
	LUA_SET_GLOBAL_ENUMCONST(state, KV_FLAG_ARRAY);

	OOLUA::register_class<IFile>(state);
	OOLUA::register_class_static<IFile>(state, "Read", L_IVirtualStream_Read);	// register using static
	OOLUA::register_class_static<IFile>(state, "Write", L_IVirtualStream_Write);

	OOLUA::register_class<IFileSystem>(state);
	OOLUA::register_class_static<IFileSystem>(state, "AddPackage", L_IFileSystem_AddPackage_NoMount);
	OOLUA::register_class_static<IFileSystem>(state, "GetFileBuffer", L_IFileSystem_GetFileBuffer);
	
	OOLUA::register_class <CFileSystemFind>(state);

	IFileSystem* fsInstance = g_fileSystem.GetInstancePtr();
	OOLUA::set_global(state, "fileSystem", fsInstance);

	// keyvalues related to FS
	OOLUA::register_class<kvpairvalue_t>(state);
	OOLUA::register_class<kvkeybase_t>(state);
	OOLUA::register_class<KeyValues>(state);

	OOLUA::set_global(state, "KV_GetValueString", L_KV_GetValueString);
	OOLUA::set_global(state, "KV_GetValueInt", L_KV_GetValueInt);
	OOLUA::set_global(state, "KV_GetValueFloat", L_KV_GetValueFloat);
	OOLUA::set_global(state, "KV_GetValueBool", L_KV_GetValueBool);
	OOLUA::set_global(state, "KV_GetVector2D", L_KV_GetVector2D);
	OOLUA::set_global(state, "KV_GetVector3D", L_KV_GetVector3D);
	OOLUA::set_global(state, "KV_GetVector4D", L_KV_GetVector4D);

	OOLUA::set_global(state, "KV_PrintSection", L_KV_PrintSection);
}

//---------------------------------------------------------------------------------------
// Vector math
//---------------------------------------------------------------------------------------

OOLUA_EXPORT_FUNCTIONS(IVector2D, set_x, set_y)
OOLUA_EXPORT_FUNCTIONS_CONST(IVector2D, get_x, get_y)

OOLUA_EXPORT_FUNCTIONS(Vector2D, set_x, set_y)
OOLUA_EXPORT_FUNCTIONS_CONST(Vector2D, get_x, get_y)

OOLUA_EXPORT_FUNCTIONS(Vector3D, set_x, set_y, set_z)
OOLUA_EXPORT_FUNCTIONS_CONST(Vector3D, get_x, get_y, get_z, xy, yz, xz)

OOLUA_EXPORT_FUNCTIONS(Vector4D, set_x, set_y, set_z, set_w)
OOLUA_EXPORT_FUNCTIONS_CONST(Vector4D, get_x, get_y, get_z, get_w, xy, xz, xw, yz, yw, zw, xyz, yzw)

OOLUA_EXPORT_FUNCTIONS(Plane, set_normal, set_offset)
OOLUA_EXPORT_FUNCTIONS_CONST(Plane, get_normal, get_offset, Distance, GetIntersectionWithRay, GetIntersectionLineFraction)

OOLUA_EXPORT_FUNCTIONS(Quaternion, set_x, set_y, set_z, set_w, normalize, fastNormalize)
OOLUA_EXPORT_FUNCTIONS_CONST(Quaternion, get_x, get_y, get_z, get_w, asVector4D, isNan)

Quaternion qconjugate(const Quaternion& quat) 
{
	return !quat;
}

OOLUA::Table qeulers(const Quaternion& q, EQuatRotationSequence seq)
{
	OOLUA::Script& state = GetLuaState();
	OOLUA::Table table = OOLUA::new_table(state);

	float res[3];
	quaternionToEulers(q, seq, res);

	table.set(1, res[0]);
	table.set(2, res[1]);
	table.set(3, res[2]);

	return table;
}

int L_qconjugate(lua_State* vm) { OOLUA_C_FUNCTION(Quaternion, qconjugate, const Quaternion &) }
int L_qslerp(lua_State* vm) { OOLUA_C_FUNCTION(Quaternion, slerp, const Quaternion &, const Quaternion &, const float) }
int L_qscerp(lua_State* vm) { OOLUA_C_FUNCTION(Quaternion, scerp, const Quaternion &, const Quaternion &, const Quaternion &, const Quaternion &, const float);}
int L_qinverse(lua_State* vm) { OOLUA_C_FUNCTION(Quaternion, inverse, const Quaternion&); }
int L_qlength(lua_State* vm) { OOLUA_C_FUNCTION(float, length, const Quaternion&);}
int L_qeulers(lua_State* vm) { OOLUA_C_FUNCTION(OOLUA::Table, qeulers, const Quaternion&, EQuatRotationSequence) }
int L_qeulersXYZ(lua_State* vm) { OOLUA_C_FUNCTION(TVec3D<float>, eulersXYZ, const Quaternion&);}
int L_qrenormalize(lua_State* vm) { OOLUA_C_FUNCTION(void, renormalize, Quaternion&);}
int L_qaxisAngle(lua_State* vm) { OOLUA_C_FUNCTION(void, axisAngle, const Quaternion&, TVec3D<float>&, float&);}
int L_qcompare(lua_State* vm) { OOLUA_C_FUNCTION(bool, compare_epsilon, const Quaternion&, const Quaternion&, const float);}
int L_qrotate(lua_State* vm) { OOLUA_C_FUNCTION(TVec3D<float>, rotateVector, const TVec3D<float>&, const Quaternion&);}
int L_qidentity(lua_State* vm) { OOLUA_C_FUNCTION(Quaternion, identity);}

OOLUA_CFUNC(VectorAngles, L_VectorAngles)

OOLUA_CFUNC(ConstrainAngle180, L_ConstrainAngle180)
OOLUA_CFUNC(ConstrainAngle360, L_ConstrainAngle360)

OOLUA_CFUNC(NormalizeAngles180, L_NormalizeAngles180)
OOLUA_CFUNC(NormalizeAngles360, L_NormalizeAngles360)

OOLUA_CFUNC(AngleDiff, L_AngleDiff)
OOLUA_CFUNC(AnglesDiff, L_AnglesDiff)

// FLOAT
int L_fract(lua_State* vm) { OOLUA_C_FUNCTION(float, fract, float) }

int L_lerp(lua_State* vm) { OOLUA_C_FUNCTION(float, lerp, const float, const float, const float) }
int L_cerp(lua_State* vm) { OOLUA_C_FUNCTION(float, cerp, const float, const float, const float, const float, float) }
int L_sign(lua_State* vm) { OOLUA_C_FUNCTION(float, sign, const float) }
int L_clamp(lua_State* vm) { OOLUA_C_FUNCTION(float, clamp, const float, const float, const float) }

// IVECTOR2D

int L_iv2d_sign(lua_State* vm) { OOLUA_C_FUNCTION(IVector2D, sign, const IVector2D&) }
int L_iv2d_clamp(lua_State* vm) { OOLUA_C_FUNCTION(IVector2D, clamp, const IVector2D&, const IVector2D&, const IVector2D&) }

// VECTOR2D

int L_v2d_lerp(lua_State* vm) { OOLUA_C_FUNCTION(Vector2D, lerp, const Vector2D&, const Vector2D&, const float) }
int L_v2d_cerp(lua_State* vm) { OOLUA_C_FUNCTION(Vector2D, cerp, const Vector2D&, const Vector2D&, const Vector2D&, const Vector2D&, float) }
int L_v2d_sign(lua_State* vm) { OOLUA_C_FUNCTION(Vector2D, sign, const Vector2D&) }
int L_v2d_clamp(lua_State* vm) { OOLUA_C_FUNCTION(Vector2D, clamp, const Vector2D&, const Vector2D&, const Vector2D&) }

int L_v2d_distance(lua_State* vm) { OOLUA_C_FUNCTION(float, distance, const Vector2D&, const Vector2D&) }
int L_v2d_length(lua_State* vm) { OOLUA_C_FUNCTION(float, length, const Vector2D&) }
int L_v2d_lengthSqr(lua_State* vm) { OOLUA_C_FUNCTION(float, lengthSqr, const Vector2D&) }
int L_v2d_dot(lua_State* vm) { OOLUA_C_FUNCTION(float, dot, const Vector2D&, const Vector2D&) }
int L_v2d_normalize(lua_State* vm) { OOLUA_C_FUNCTION(Vector2D, fastNormalize, const Vector2D&) }

int L_v2d_lineProjection(lua_State* vm) { OOLUA_C_FUNCTION(float, lineProjection, const Vector2D&, const Vector2D&, const Vector2D&) }

// VECTOR3D

int L_v3d_lerp(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, lerp, const Vector3D&, const Vector3D&, const float) }
int L_v3d_cerp(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, cerp, const Vector3D&, const Vector3D&, const Vector3D&, const Vector3D&, float) }
int L_v3d_sign(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, sign, const Vector3D&) }
int L_v3d_clamp(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, clamp, const Vector3D&, const Vector3D&, const Vector3D&) }

int L_v3d_distance(lua_State* vm) { OOLUA_C_FUNCTION(float, distance, const Vector3D&, const Vector3D&) }
int L_v3d_length(lua_State* vm) { OOLUA_C_FUNCTION(float, length, const Vector3D&) }
int L_v3d_lengthSqr(lua_State* vm) { OOLUA_C_FUNCTION(float, lengthSqr, const Vector3D&) }
int L_v3d_dot(lua_State* vm) { OOLUA_C_FUNCTION(float, dot, const Vector3D&, const Vector3D&) }
int L_v3d_normalize(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, fastNormalize, const Vector3D&) }

int L_v3d_lineProjection(lua_State* vm) { OOLUA_C_FUNCTION(float, lineProjection, const Vector3D&, const Vector3D&, const Vector3D&) }

int L_v3d_cross(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, cross, const Vector3D&, const Vector3D&) }
int L_v3d_reflect(lua_State* vm) { OOLUA_C_FUNCTION(Vector3D, reflect, const Vector3D&, const Vector3D&) }

void Math_InitBinding(lua_State* state)
{
	LUA_SET_GLOBAL_ENUMCONST(state, QuatRot_zyx);
	LUA_SET_GLOBAL_ENUMCONST(state, QuatRot_zxy);
	LUA_SET_GLOBAL_ENUMCONST(state, QuatRot_yxz);
	LUA_SET_GLOBAL_ENUMCONST(state, QuatRot_yzx);
	LUA_SET_GLOBAL_ENUMCONST(state, QuatRot_xyz);
	LUA_SET_GLOBAL_ENUMCONST(state, QuatRot_xzy);

	OOLUA::register_class<Vector2D>(state);
	OOLUA::register_class<IVector2D>(state);
	OOLUA::register_class<Vector3D>(state);
	OOLUA::register_class<Vector4D>(state);
	OOLUA::register_class<Plane>(state);
	OOLUA::register_class<Quaternion>(state);

	OOLUA::set_global(state, "f_fract", L_fract);
	OOLUA::set_global(state, "f_lerp", L_lerp);
	OOLUA::set_global(state, "f_cerp", L_cerp);
	OOLUA::set_global(state, "f_sign", L_sign);
	OOLUA::set_global(state, "f_clamp", L_clamp);
	OOLUA::set_global(state, "v2d_lerp", L_v2d_lerp);
	OOLUA::set_global(state, "v2d_cerp", L_v2d_cerp);
	OOLUA::set_global(state, "v2d_sign", L_v2d_sign);
	OOLUA::set_global(state, "v2d_clamp", L_v2d_clamp);

	OOLUA::set_global(state, "iv2d_sign", L_iv2d_sign);
	OOLUA::set_global(state, "iv2d_clamp", L_iv2d_clamp);

	OOLUA::set_global(state, "v2d_distance", L_v2d_distance);
	OOLUA::set_global(state, "v2d_length", L_v2d_length);
	OOLUA::set_global(state, "v2d_lengthSqr", L_v2d_lengthSqr);
	OOLUA::set_global(state, "v2d_dot", L_v2d_dot);
	OOLUA::set_global(state, "v2d_normalize", L_v2d_normalize);
	OOLUA::set_global(state, "v2d_lineProjection", L_v2d_lineProjection);

	OOLUA::set_global(state, "v3d_lerp", L_v3d_lerp);
	OOLUA::set_global(state, "v3d_cerp", L_v3d_cerp);
	OOLUA::set_global(state, "v3d_sign", L_v3d_sign);
	OOLUA::set_global(state, "v3d_clamp", L_v3d_clamp);

	OOLUA::set_global(state, "v3d_distance", L_v3d_distance);
	OOLUA::set_global(state, "v3d_length", L_v3d_length);
	OOLUA::set_global(state, "v3d_lengthSqr", L_v3d_lengthSqr);
	OOLUA::set_global(state, "v3d_dot", L_v3d_dot);
	OOLUA::set_global(state, "v3d_normalize", L_v3d_normalize);
	OOLUA::set_global(state, "v3d_lineProjection", L_v3d_lineProjection);
	OOLUA::set_global(state, "v3d_cross", L_v3d_cross);
	OOLUA::set_global(state, "v3d_reflect", L_v3d_reflect);

	OOLUA::set_global(state, "qinverse", L_qinverse);
	OOLUA::set_global(state, "qconjugate", L_qconjugate);
	OOLUA::set_global(state, "qslerp", L_qslerp);
	OOLUA::set_global(state, "qscerp", L_qscerp);
	OOLUA::set_global(state, "qlength", L_qlength);
	OOLUA::set_global(state, "qeulersSel", L_qeulers);
	OOLUA::set_global(state, "qeulers", L_qeulersXYZ);
	OOLUA::set_global(state, "qrenormalize", L_qrenormalize);
	OOLUA::set_global(state, "qaxisAngle", L_qaxisAngle);
	OOLUA::set_global(state, "qcompare", L_qcompare);
	OOLUA::set_global(state, "qrotate", L_qrotate);
	OOLUA::set_global(state, "qidentity", L_qidentity);

	OOLUA::set_global(state, "ConstrainAngle180", L_ConstrainAngle180);
	OOLUA::set_global(state, "ConstrainAngle360", L_ConstrainAngle360);

	OOLUA::set_global(state, "NormalizeAngles180", L_NormalizeAngles180);
	OOLUA::set_global(state, "NormalizeAngles360", L_NormalizeAngles360);

	OOLUA::set_global(state, "AngleDiff", L_AngleDiff);
	OOLUA::set_global(state, "AnglesDiff", L_AnglesDiff);

	OOLUA::set_global(state, "VectorAngles", L_VectorAngles);
}

//---------------------------------------------------------------------------------------
// EqUI
//---------------------------------------------------------------------------------------

OOLUA_EXPORT_NO_FUNCTIONS(ILocToken)
OOLUA_EXPORT_NO_FUNCTIONS(IEqFont)

OOLUA_EXPORT_FUNCTIONS(CEqFontCache)
OOLUA_EXPORT_FUNCTIONS_CONST(CEqFontCache, GetFont)

ILocToken* LocalizedToken(char* pszToken)
{
	ILocToken* tok = g_localizer->GetToken(pszToken);
	return tok;
}

int L_LocalizedToken(lua_State* vm) { OOLUA_C_FUNCTION(OOLUA::maybe_null<ILocToken*>, LocalizedToken, char*) }

void AddLanguageFile(char* filenamePrefix)
{
	return g_localizer->AddTokensFile(filenamePrefix);
}

OOLUA_CFUNC(AddLanguageFile, L_AddLanguageFile)

OOLUA_EXPORT_FUNCTIONS(equi::IUIControl, 

	InitFromKeyValues,
	SetName,
	SetLabel,
	Show,
	Hide,
	SetVisible,
	SetSelfVisible,
	Enable,
	SetSize,
	SetPosition,
	//SetRectangle,
	SetTextColor,
	SetTransform,
	AddChild,
	RemoveChild,
	FindChild,
	ClearChilds
)

OOLUA_EXPORT_FUNCTIONS_CONST(equi::IUIControl, 
	GetName,
	GetLabel,
	IsVisible,
	IsSelfVisible,
	IsEnabled,
	GetSize,
	GetPosition,
	//GetRectangle,
	//GetClientRectangle,
	GetTextColor,
	GetClassname,
	GetParent
	//GetFont
)

OOLUA_EXPORT_FUNCTIONS(equi::Panel, 
	SetColor, SetSelectionColor, 
	CenterOnScreen
)
OOLUA_EXPORT_FUNCTIONS_CONST(equi::Panel, 
	GetColor, GetSelectionColor
)

OOLUA_EXPORT_FUNCTIONS(equi::Image,
	SetMaterial,
	SetColor
)
OOLUA_EXPORT_FUNCTIONS_CONST(equi::Image, GetColor)

OOLUA_EXPORT_FUNCTIONS(equi::CUIManager,

	//RegisterFactory,
	CreateElement,
	AddPanel,
	DestroyPanel,
	BringToTop,
	//SetViewFrame,
	SetFocus

)
OOLUA_EXPORT_FUNCTIONS_CONST(equi::CUIManager,
	GetRootPanel,
	FindPanel,
	GetTopPanel,
	// GetViewFrame,
	GetScreenSize,
	GetFocus,
	GetMouseOver,
	IsWindowsVisible,
	GetDefaultFont
)

// Cast functions
int L_equi_castto_panel( lua_State* vm ) { OOLUA_C_FUNCTION(OOLUA::maybe_null<equi::Panel*>, equi::DynamicCast, equi::IUIControl*) }
//int L_equi_castto_label( lua_State* vm ) { OOLUA_C_FUNCTION(OOLUA::maybe_null<equi::Label*>, equi::DynamicCast, equi::IUIControl*) }
int L_equi_castto_image( lua_State* vm ) { OOLUA_C_FUNCTION(OOLUA::maybe_null<equi::Image*>, equi::DynamicCast, equi::IUIControl*) }

void EqUI_InitBinding(lua_State* state)
{
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_STYLE_REGULAR);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_STYLE_BOLD);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_STYLE_ITALIC);

	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_ALIGN_LEFT);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_ALIGN_RIGHT);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_ALIGN_HCENTER);

	// Localizer is first
	OOLUA::register_class<ILocToken>(state);
	OOLUA::set_global(state, "LocalizedToken", L_LocalizedToken);
	OOLUA::set_global(state, "AddLanguageFile", L_AddLanguageFile);

	// EqUI
	OOLUA::register_class<IEqFont>(state);
	OOLUA::register_class<CEqFontCache>(state);

	OOLUA::register_class<equi::IUIControl>(state);
	OOLUA::register_class<equi::Panel>(state);
	OOLUA::register_class<equi::Image>(state);
	OOLUA::register_class<equi::CUIManager>(state);

	OOLUA::Table equiCastFuncsTab = OOLUA::new_table(state);
	equiCastFuncsTab.set("panel", L_equi_castto_panel);
	equiCastFuncsTab.set("image", L_equi_castto_image);
	//equiCastFuncsTab.set("label", L_equi_castto_label);

	

	OOLUA::set_global(state, "equi_cast", equiCastFuncsTab);
	OOLUA::set_global(state, "equi", equi::Manager);
}

#include "in_keys_ident.h"

void Input_InitBinding(lua_State* state)
{
	OOLUA::Table inputKeys = OOLUA::new_table(state);

	for (keyNameMap_t* kn = s_keyMapList; kn->name; kn++)
	{
		inputKeys.set(kn->name, kn->keynum);
	}

	OOLUA::set_global(state, "inputMap", inputKeys);
}

// Engine state
int L_ChangeStateType(lua_State* vm) { OOLUA_C_FUNCTION(void, EqStateMgr::ChangeStateType, int) }
int L_SetCurrentStateType(lua_State* vm) { OOLUA_C_FUNCTION(void, EqStateMgr::SetCurrentStateType, int) }
int L_GetCurrentStateType(lua_State* vm) { OOLUA_C_FUNCTION(int, EqStateMgr::GetCurrentStateType) }
int L_ScheduleNextStateType(lua_State* vm) { OOLUA_C_FUNCTION(void, EqStateMgr::ScheduleNextStateType, int) }

//---------------------------------------------------------------------------------------
// Console
//---------------------------------------------------------------------------------------

OOLUA_EXPORT_FUNCTIONS(ConCommandBase)
OOLUA_EXPORT_FUNCTIONS_CONST(ConCommandBase, GetName, GetDesc, GetFlags, IsConVar, IsConCommand, IsRegistered)

OOLUA_EXPORT_FUNCTIONS(ConCommand)
OOLUA_EXPORT_FUNCTIONS_CONST(ConCommand)

OOLUA_EXPORT_FUNCTIONS(ConVar, RevertToDefaultValue, SetString, SetFloat, SetInt, SetBool)
OOLUA_EXPORT_FUNCTIONS_CONST(ConVar, HasClamp, GetMinClamp, GetMaxClamp, GetFloat, GetString, GetInt, GetBool)

OOLUA_EXPORT_FUNCTIONS(ICommandLineParse)
OOLUA_EXPORT_FUNCTIONS_CONST(ICommandLineParse, FindArgument, GetArgumentString, GetArgumentsOf, GetArgumentCount)

ConVar* Lua_Console_FindCvar(const char* name)
{
	return (ConVar*)g_sysConsole->FindCvar(name);
}

ConCommand* Lua_Console_FindCommand(const char* name)
{
	return (ConCommand*)g_sysConsole->FindCommand(name);
}

void Lua_Console_ExecuteString(const char* cmd)
{
	g_sysConsole->SetCommandBuffer(cmd);
	g_sysConsole->ExecuteCommandBuffer();
}

struct luaCmdFuncRef_t
{
	const char* name;
	OOLUA::Lua_func_ref funcRef;
	OOLUA::Lua_func_ref variantsFuncRef;
};

DkList<luaCmdFuncRef_t> g_luaCmdFuncRefs;

int FindCmdFunc(const char* cmdName)
{
	for (int i = 0; i < g_luaCmdFuncRefs.numElem(); i++)
	{
		if (g_luaCmdFuncRefs[i].name == cmdName) // because I already adjusted same addresses
			return i;
	}

	return -1;
}

void luaCommandVariantsFunc(const ConCommandBase* base, DkList<EqString>& variants, const char* query)
{
	int funcIdx = FindCmdFunc(base->GetName());
	ASSERT(funcIdx >= 0);

	luaCmdFuncRef_t& ref = g_luaCmdFuncRefs[funcIdx];

	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if (!ref.variantsFuncRef.push(state))
		return;

	//if (!argTable.push_on_stack(state))
	if (!OOLUA::push(state, query))
		MsgError("luaCommandVariantsFunc can't push 'query' on stack\n");

	int res = lua_pcall(state, 1, 1, 0);

	if (res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(state);
		MsgError(":%s (ConCommandBase variants) error:\n %s\n", base->GetName(), OOLUA::get_last_error(state).c_str());
	}

	OOLUA::Table results;
	if (!OOLUA::pull(state, results))
		MsgError("luaCommandVariantsFunc can't pull table from stack\n");

	{
		oolua_ipairs(results)

		std::string elem;
		results.safe_at(_i_index_, elem);

		variants.append(elem.c_str());

		oolua_ipairs_end()
	}
}

void S_ConCommandBase_SetVariantsCallback(ConCommandBase* base, OOLUA::Lua_func_ref variantsFunc)
{
	int funcIdx = FindCmdFunc(base->GetName());
	ASSERT(funcIdx >= 0);

	luaCmdFuncRef_t& ref = g_luaCmdFuncRefs[funcIdx];
	ref.variantsFuncRef = variantsFunc;

	if(variantsFunc.valid())
		base->SetVariantsCallback(luaCommandVariantsFunc);
	else
		base->SetVariantsCallback(nullptr);
}

OOLUA_CFUNC(S_ConCommandBase_SetVariantsCallback, L_ConCommandBase_SetVariantsCallback)

DECLARE_CONCOMMAND_FN(luaConCommandHandler)
{
	int funcIdx = FindCmdFunc(cmd->GetName());
	ASSERT(funcIdx >= 0);

	luaCmdFuncRef_t& ref = g_luaCmdFuncRefs[funcIdx];

	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	// make argument table
	OOLUA::Table argTable = OOLUA::new_table(state);

	for (int i = 0; i < CMD_ARGC; i++)
		argTable.set(i + 1, CMD_ARGV(i).c_str());

	if (!ref.funcRef.push(state))
		return;

	if (!argTable.push_on_stack(state))
		MsgError("luaConCommandHandler can't push table on stack\n");

	int res = lua_pcall(state, 1, 0, 0);

	if (res != 0)
	{
		OOLUA::INTERNAL::set_error_from_top_of_stack_and_pop_the_error(state);
		MsgError(":%s (ConCommand) error:\n %s\n", cmd->GetName(), OOLUA::get_last_error(state).c_str());
	}
}

ConCommand* Lua_Console_CreateCommand(char const* name, OOLUA::Lua_func_ref cmdFunc, char const* desc, int flags)
{
	// register con. command function reference
	ASSERTMSG(cmdFunc.valid() == true, varargs("Not valid function for Lua ConCommand %s", name));

	luaCmdFuncRef_t ref;
	ref.name = xstrdup(name);
	ref.funcRef = cmdFunc;

	g_luaCmdFuncRefs.append(ref);

	return new ConCommand(ref.name, CONCOMMAND_FN(luaConCommandHandler), xstrdup(desc), flags);
}

ConVar* Lua_Console_CreateCvar(char const* name, char const* value, char const* desc, int flags)
{
	return new ConVar(xstrdup(name), xstrdup(value), xstrdup(desc), flags);
}

void Lua_Console_RemoveCommandBase(ConCommandBase* cmdbase)
{
	if (cmdbase->IsConCommand())
	{
		int funcIdx = FindCmdFunc(cmdbase->GetName());
		ASSERT(funcIdx >= 0);

		g_luaCmdFuncRefs.fastRemoveIndex(funcIdx);
	}

	g_sysConsole->UnregisterCommand(cmdbase);

	cmdbase->LuaCleanup();
	delete cmdbase;
}

void Lua_Console_RegisterCommandBase(ConCommandBase* cmdbase)
{
	g_sysConsole->RegisterCommand(cmdbase);
}

void Lua_Console_UnregisterCommandBase(ConCommandBase* cmdbase)
{
	g_sysConsole->UnregisterCommand(cmdbase);
}

int LLua_Console_FindCvar(lua_State* vm)
{
	OOLUA_C_FUNCTION(OOLUA::maybe_null<ConVar*>, Lua_Console_FindCvar, const char*)
}

int LLua_Console_FindCommand(lua_State* vm)
{
	OOLUA_C_FUNCTION(OOLUA::maybe_null<ConCommand*>, Lua_Console_FindCommand, const char*)
}

OOLUA_CFUNC(Lua_Console_ExecuteString, LLua_Console_ExecuteString)
OOLUA_CFUNC(Lua_Console_CreateCommand, LLua_Console_CreateCommand)
OOLUA_CFUNC(Lua_Console_CreateCvar, LLua_Console_CreateCvar)
OOLUA_CFUNC(Lua_Console_RemoveCommandBase, LLua_Console_RemoveCommandBase)
OOLUA_CFUNC(Lua_Console_RegisterCommandBase, LLua_Console_RegisterCommandBase)
OOLUA_CFUNC(Lua_Console_UnregisterCommandBase, LLua_Console_UnregisterCommandBase)

void Console_InitBinding(lua_State* state)
{
	LUA_SET_GLOBAL_ENUMCONST(state, CV_UNREGISTERED);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_CHEAT);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_INITONLY);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_INVISIBLE);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_ARCHIVE);

	OOLUA::register_class<ConCommandBase>(state);
	OOLUA::register_class<ConVar>(state);
	OOLUA::register_class<ConCommand>(state);

	OOLUA::register_class_static<ConVar>(state, "SetVariantsCallback", L_ConCommandBase_SetVariantsCallback);
	OOLUA::register_class_static<ConCommand>(state, "SetVariantsCallback", L_ConCommandBase_SetVariantsCallback);
	
	OOLUA::register_class<ICommandLineParse>(state);

	OOLUA::Table consoleTab = OOLUA::new_table(state);
	consoleTab.set("FindCvar", LLua_Console_FindCvar);
	consoleTab.set("FindCommand", LLua_Console_FindCommand);
	consoleTab.set("ExecuteString", LLua_Console_ExecuteString);

	consoleTab.set("CreateCommand", LLua_Console_CreateCommand);
	consoleTab.set("CreateCvar", LLua_Console_CreateCvar);
	consoleTab.set("RemoveCommandBase", LLua_Console_RemoveCommandBase);
	consoleTab.set("RegisterCommandBase", LLua_Console_RegisterCommandBase);
	consoleTab.set("UnregisterCommandBase", LLua_Console_UnregisterCommandBase);

	ICommandLineParse* cmdLineInstance = g_cmdLine.GetInstancePtr();

	OOLUA::set_global(state, "console", consoleTab);
	OOLUA::set_global(state, "cmdline", cmdLineInstance);
}


//---------------------------------------------------------------------------------------
// MAIN
//---------------------------------------------------------------------------------------

OOLUA_EXPORT_FUNCTIONS(IDebugOverlay, Line3D, Box3D, Sphere3D, Polygon3D)
OOLUA_EXPORT_FUNCTIONS_CONST(IDebugOverlay)

void S_IDebugOverlay_Text(IDebugOverlay* _self, const ColorRGBA &color, char const* text)
{
	_self->Text(color, text);
}
OOLUA_CFUNC(S_IDebugOverlay_Text, L_IDebugOverlay_Text)

void S_IDebugOverlay_TextFadeOut(IDebugOverlay* _self, int position, const ColorRGBA &color, float fFadeTime, char const* text)
{
	_self->TextFadeOut(position, color, fFadeTime, text);
}
OOLUA_CFUNC(S_IDebugOverlay_TextFadeOut, L_IDebugOverlay_TextFadeOut)

void S_IDebugOverlay_Text3D(IDebugOverlay* _self, const Vector3D &origin, float distance, const ColorRGBA &color, float fTime, char const* text)
{
	_self->Text3D(origin, distance, color, fTime, text);
}
OOLUA_CFUNC(S_IDebugOverlay_Text3D, L_IDebugOverlay_Text3D)

OOLUA_EXPORT_FUNCTIONS(ISoundController, StartSound, Play, Pause, Stop, SetPitch, SetVolume, SetOrigin, SetVelocity)
OOLUA_EXPORT_FUNCTIONS_CONST(ISoundController, IsStopped)

OOLUA_EXPORT_FUNCTIONS(EmitParams, set_sampleId)
OOLUA_EXPORT_FUNCTIONS_CONST(EmitParams, get_sampleId)

OOLUA_EXPORT_FUNCTIONS(CSoundEmitterSystem, Precache, Emit, Emit2D, CreateController, RemoveController, LoadScript)
OOLUA_EXPORT_FUNCTIONS_CONST(CSoundEmitterSystem)

OOLUA_EXPORT_FUNCTIONS(Networking::CNetMessageBuffer,
	WriteByte, WriteUByte, WriteInt16, WriteUInt16, WriteInt, WriteUInt, WriteBool, WriteFloat, WriteVector2D,
	WriteVector3D, WriteVector4D,
	ReadByte, ReadUByte, ReadInt16, ReadUInt16, ReadInt, ReadUInt, ReadBool, ReadFloat, ReadVector2D,
	ReadVector3D, ReadVector4D, WriteString, WriteNetBuffer, WriteKeyValues, ReadKeyValues)

OOLUA_EXPORT_FUNCTIONS_CONST(Networking::CNetMessageBuffer, GetMessageLength, GetClientID)

// since direct ReadString is unsafe, we're overriding it with wrapper
std::string S_CNetMessageBuffer_ReadString(Networking::CNetMessageBuffer* buf)
{
	EqString str = buf->ReadString();
	return str.c_str();
}
OOLUA_CFUNC(S_CNetMessageBuffer_ReadString, L_CNetMessageBuffer_ReadString)

OOLUA_EXPORT_FUNCTIONS(Networking::CNetworkThread, SendData, SendEvent, SendWaitDataEvent)
OOLUA_EXPORT_FUNCTIONS_CONST(Networking::CNetworkThread)

double S_Host_GetFrameTime()
{
	return g_pHost->GetFrameTime();
}

void S_Host_RequestTextInput()
{
	g_pHost->RequestTextInput();
}

void S_Host_ReleaseTextInput()
{
	g_pHost->ReleaseTextInput();
}

bool S_Host_IsTextInputShown()
{
	return g_pHost->IsTextInputShown();
}

OOLUA_CFUNC(S_Host_GetFrameTime, L_Host_GetFrameTime)
OOLUA_CFUNC(S_Host_RequestTextInput, L_Host_RequestTextInput)
OOLUA_CFUNC(S_Host_ReleaseTextInput, L_Host_ReleaseTextInput)
OOLUA_CFUNC(S_Host_IsTextInputShown, L_Host_IsTextInputShown)

bool LuaBinding_InitEngineBindings(lua_State* state)
{
	// base modules
	DebugMessages_InitBinding(state);
	Console_InitBinding(state);
	FileSystem_InitBinding(state);
	Math_InitBinding(state);
	EqUI_InitBinding(state);
	Input_InitBinding(state);
	
	// debug overlay
	OOLUA::register_class<IDebugOverlay>(state);
	OOLUA::register_class_static<IDebugOverlay>(state, "Text", L_IDebugOverlay_Text);
	OOLUA::register_class_static<IDebugOverlay>(state, "TextFadeOut", L_IDebugOverlay_TextFadeOut);
	OOLUA::register_class_static<IDebugOverlay>(state, "Text3D", L_IDebugOverlay_Text3D);

	OOLUA::set_global(state, "debugoverlay", debugoverlay);

	// sound emitter system
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_OCCLUSION);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_ROOM_OCCLUSION);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_FORCE_CACHED);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_FORCE_2D);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_STARTSILENT);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_START_ON_UPDATE);

	OOLUA::register_class<ISoundController>(state);
	OOLUA::register_class<EmitParams>(state);
	OOLUA::register_class<CSoundEmitterSystem>(state);
	OOLUA::set_global(state, "sounds", g_sounds);

	// networking classes
	OOLUA::register_class<Networking::CNetMessageBuffer>(state);
	OOLUA::register_class_static<Networking::CNetMessageBuffer>(state, "ReadString", L_CNetMessageBuffer_ReadString);

	OOLUA::register_class<Networking::CNetworkThread>(state);
	
	// engine state manager
	{
		OOLUA::Table eqStateTable = OOLUA::new_table(state);
		eqStateTable.set("ChangeStateType", L_ChangeStateType);
		eqStateTable.set("SetCurrentStateType", L_SetCurrentStateType);
		eqStateTable.set("GetCurrentStateType", L_GetCurrentStateType);
		eqStateTable.set("ScheduleNextStateType", L_ScheduleNextStateType);
		OOLUA::set_global(state, "EqStateMgr", eqStateTable);
	}

	// engine host
	{
		OOLUA::Table hostTable = OOLUA::new_table(state);
		hostTable.set("GetFrameTime", L_Host_GetFrameTime);
		hostTable.set("RequestTextInput", L_Host_RequestTextInput);
		hostTable.set("ReleaseTextInput", L_Host_ReleaseTextInput);
		hostTable.set("IsTextInputShown", L_Host_IsTextInputShown);
		OOLUA::set_global(state, "host", hostTable);
	}

	// eqCore
	{
		OOLUA::Table eqCoreTable = OOLUA::new_table(state);
		eqCoreTable.set("GetConfig", L_DkCore_GetConfig);
		OOLUA::set_global(state, "core", eqCoreTable);
	}

	return true;
}

void LuaBinding_ShutdownEngineBindings()
{
	g_luaCmdFuncRefs.clear();
}

bool LuaBinding_ConsoleHandler(const char* cmdText)
{
	EqString cmdFullText(cmdText);

	if (*cmdText == '=')
		cmdFullText = varargs("ConsolePrint(%s)", cmdText + 1);

	if (!EqLua::LuaBinding_DoBuffer(GetLuaState(), cmdFullText.c_str(), cmdFullText.Length(), "console"))
		MsgError("%s\n", OOLUA::get_last_error(GetLuaState()).c_str());

	return true;
}