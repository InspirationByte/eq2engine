//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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

void WMsg(const char* str)
{
	Msg(str);
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
OOLUA_CFUNC( WMsgWarning, LMsgWarning)
OOLUA_CFUNC( WMsgError, LMsgError)
OOLUA_CFUNC( WMsgAccept, LMsgAccept)

//--------------------------------------------------------------------------

OOLUA_EXPORT_FUNCTIONS(ConVar, RevertToDefaultValue, SetString,SetFloat,SetInt,SetBool)
OOLUA_EXPORT_FUNCTIONS_CONST(ConVar, GetName, HasClamp, GetMinClamp, GetMaxClamp, GetFloat, GetString, GetInt, GetBool)

OOLUA_EXPORT_FUNCTIONS(Vector2D, set_x, set_y)
OOLUA_EXPORT_FUNCTIONS_CONST(Vector2D, get_x, get_y)

OOLUA_EXPORT_FUNCTIONS(Vector3D, set_x, set_y, set_z)
OOLUA_EXPORT_FUNCTIONS_CONST(Vector3D, get_x, get_y, get_z, xy, yz, xz)

OOLUA_EXPORT_FUNCTIONS(Vector4D, set_x, set_y, set_z, set_w)
OOLUA_EXPORT_FUNCTIONS_CONST(Vector4D, get_x, get_y, get_z, get_w, xy, xz, xw, yz, yw, zw, xyz, yzw)

OOLUA_EXPORT_FUNCTIONS(Plane, set_normal, set_offset)
OOLUA_EXPORT_FUNCTIONS_CONST(Plane, get_normal, get_offset, Distance, GetIntersectionWithRay, GetIntersectionLineFraction)

OOLUA_EXPORT_FUNCTIONS(IDebugOverlay, Line3D, Box3D, Polygon3D)
OOLUA_EXPORT_FUNCTIONS_CONST(IDebugOverlay)

//----------

OOLUA_EXPORT_FUNCTIONS(ISoundController, Start, Pause, Stop, SetPitch, SetVolume, SetOrigin, SetVelocity)
OOLUA_EXPORT_FUNCTIONS_CONST(ISoundController, IsStopped)

OOLUA_EXPORT_FUNCTIONS(EmitParams)
OOLUA_EXPORT_FUNCTIONS_CONST(EmitParams)

OOLUA_EXPORT_FUNCTIONS(CSoundEmitterSystem, Precache, Emit, Emit2D, CreateController, RemoveController)
OOLUA_EXPORT_FUNCTIONS_CONST(CSoundEmitterSystem)

OOLUA_EXPORT_FUNCTIONS(ILocToken)
OOLUA_EXPORT_FUNCTIONS_CONST(ILocToken)

OOLUA_EXPORT_FUNCTIONS(IEqFont)
OOLUA_EXPORT_FUNCTIONS_CONST(IEqFont)

OOLUA_EXPORT_FUNCTIONS(CEqFontCache)
OOLUA_EXPORT_FUNCTIONS_CONST(CEqFontCache, GetFont)

OOLUA_EXPORT_FUNCTIONS(Networking::CNetMessageBuffer,
										WriteByte, WriteUByte, WriteInt16, WriteUInt16, WriteInt, WriteUInt, WriteBool, WriteFloat, WriteVector2D,
										WriteVector3D, WriteVector4D,
										ReadByte, ReadUByte, ReadInt16, ReadUInt16, ReadInt, ReadUInt, ReadBool, ReadFloat, ReadVector2D,
										ReadVector3D, ReadVector4D, /*WriteString, ReadString,*/ WriteNetBuffer/*, WriteKeyValues, ReadKeyValues*/)
OOLUA_EXPORT_FUNCTIONS_CONST(Networking::CNetMessageBuffer, GetMessageLength, GetClientID)

OOLUA_EXPORT_FUNCTIONS(Networking::CNetworkThread,	SendData, SendEvent, SendWaitDataEvent)
OOLUA_EXPORT_FUNCTIONS_CONST(Networking::CNetworkThread)


OOLUA_EXPORT_FUNCTIONS(kvkeybase_t,
	Cleanup,
	ClearValues,
	SetName,
	AddKeyBase,
	SetKey,
	RemoveKeyBase,
	SetValue,
	SetValueByIndex,
	AppendValue,
	MergeFrom
)

OOLUA_EXPORT_FUNCTIONS_CONST(kvkeybase_t, FindKeyBase, IsSection, IsArray, IsDefinition)

OOLUA_EXPORT_FUNCTIONS(KeyValues, LoadFile, SaveFile, GetRoot)

OOLUA_EXPORT_FUNCTIONS_CONST(KeyValues)

// safe and fast value returner
OOLUA_CFUNC(KV_GetValueString, L_KV_GetValueString)
OOLUA_CFUNC(KV_GetValueInt, L_KV_GetValueInt)
OOLUA_CFUNC(KV_GetValueFloat, L_KV_GetValueFloat)
OOLUA_CFUNC(KV_GetValueBool, L_KV_GetValueBool)

OOLUA_CFUNC(KV_GetVector2D, L_KV_GetVector2D)
OOLUA_CFUNC(KV_GetVector3D, L_KV_GetVector3D)
OOLUA_CFUNC(KV_GetVector4D, L_KV_GetVector4D)

OOLUA_CFUNC(VectorAngles, L_VectorAngles)

ILocToken* LocalizedToken( char* pszToken )
{
	ILocToken* tok = g_localizer->GetToken( pszToken );
	return tok;
}

void AddLanguageFile( char* filenamePrefix )
{
	return g_localizer->AddTokensFile( filenamePrefix );
}

OOLUA_CFUNC(LocalizedToken, L_LocalizedToken)
OOLUA_CFUNC(AddLanguageFile, L_AddLanguageFile)

OOLUA_CFUNC(ConstrainAngle180,L_ConstrainAngle180)
OOLUA_CFUNC(ConstrainAngle360,L_ConstrainAngle360)

OOLUA_CFUNC(NormalizeAngles180,L_NormalizeAngles180)
OOLUA_CFUNC(NormalizeAngles360,L_NormalizeAngles360)

OOLUA_CFUNC(AngleDiff,L_AngleDiff)
OOLUA_CFUNC(AnglesDiff,L_AnglesDiff)




// FLOAT
int L_fract( lua_State* vm )			{ OOLUA_C_FUNCTION(float,fract,float) }

int L_lerp( lua_State* vm )				{ OOLUA_C_FUNCTION(float,lerp,const float, const float, const float) }
int L_cerp( lua_State* vm )				{ OOLUA_C_FUNCTION(float,cerp,const float,const float,const float,const float, float) }
int L_sign( lua_State* vm )				{ OOLUA_C_FUNCTION(float,sign,const float) }
int L_clamp( lua_State* vm )			{ OOLUA_C_FUNCTION(float,clamp,const float, const float, const float) }

// VECTOR2D

int L_v2d_lerp( lua_State* vm )			{ OOLUA_C_FUNCTION(Vector2D,lerp,const Vector2D&, const Vector2D&, const float) }
int L_v2d_cerp( lua_State* vm )			{ OOLUA_C_FUNCTION(Vector2D,cerp,const Vector2D&,const Vector2D&,const Vector2D&,const Vector2D&, float) }
int L_v2d_sign( lua_State* vm )			{ OOLUA_C_FUNCTION(Vector2D,sign,const Vector2D&) }
int L_v2d_clamp( lua_State* vm )		{ OOLUA_C_FUNCTION(Vector2D,clamp,const Vector2D&, const Vector2D&, const Vector2D&) }

int L_v2d_distance( lua_State* vm )		{ OOLUA_C_FUNCTION(float,distance,const Vector2D&, const Vector2D&) }
int L_v2d_length( lua_State* vm )		{ OOLUA_C_FUNCTION(float,length,const Vector2D&) }
int L_v2d_lengthSqr( lua_State* vm )	{ OOLUA_C_FUNCTION(float,lengthSqr,const Vector2D&) }
int L_v2d_dot( lua_State* vm )			{ OOLUA_C_FUNCTION(float,dot,const Vector2D&, const Vector2D&) }
int L_v2d_normalize( lua_State* vm )	{ OOLUA_C_FUNCTION(Vector2D,fastNormalize,const Vector2D&) }

int L_v2d_lineProjection( lua_State* vm )	{ OOLUA_C_FUNCTION(float,lineProjection,const Vector2D&,const Vector2D&, const Vector2D&) }

// VECTOR3D

int L_v3d_lerp( lua_State* vm )			{ OOLUA_C_FUNCTION(Vector3D,lerp,const Vector3D&, const Vector3D&, const float) }
int L_v3d_cerp( lua_State* vm )			{ OOLUA_C_FUNCTION(Vector3D,cerp,const Vector3D&,const Vector3D&,const Vector3D&,const Vector3D&, float) }
int L_v3d_sign( lua_State* vm )			{ OOLUA_C_FUNCTION(Vector3D,sign,const Vector3D&) }
int L_v3d_clamp( lua_State* vm )		{ OOLUA_C_FUNCTION(Vector3D,clamp,const Vector3D&, const Vector3D&, const Vector3D&) }

int L_v3d_distance( lua_State* vm )		{ OOLUA_C_FUNCTION(float,distance,const Vector3D&, const Vector3D&) }
int L_v3d_length( lua_State* vm )		{ OOLUA_C_FUNCTION(float,length,const Vector3D&) }
int L_v3d_lengthSqr( lua_State* vm )	{ OOLUA_C_FUNCTION(float,lengthSqr,const Vector3D&) }
int L_v3d_dot( lua_State* vm )			{ OOLUA_C_FUNCTION(float,dot,const Vector3D&, const Vector3D&) }
int L_v3d_normalize( lua_State* vm )	{ OOLUA_C_FUNCTION(Vector3D,fastNormalize,const Vector3D&) }

int L_v3d_lineProjection( lua_State* vm )	{ OOLUA_C_FUNCTION(float,lineProjection,const Vector3D&,const Vector3D&, const Vector3D&) }

int L_v3d_cross( lua_State* vm )		{ OOLUA_C_FUNCTION(Vector3D,cross,const Vector3D&, const Vector3D&) }
int L_v3d_reflect( lua_State* vm )		{ OOLUA_C_FUNCTION(Vector3D,reflect,const Vector3D&, const Vector3D&) }

// привязка LUA
void DebugMessages_InitBinding(lua_State* state)
{
	OOLUA::set_global(state, "Msg", LMsg);
	OOLUA::set_global(state, "MsgWarning", LMsgWarning);
	OOLUA::set_global(state, "MsgError", LMsgError);
	OOLUA::set_global(state, "MsgAccept", LMsgAccept);
}

ConVar* Lua_Console_FindCvar(const char* name)
{
	return (ConVar*)GetCvars()->FindCvar(name);
}

void Lua_Console_ExecuteString(const char* cmd)
{
	GetCommandAccessor()->SetCommandBuffer(cmd);
	GetCommandAccessor()->ExecuteCommandBuffer();
}

int LLua_Console_FindCvar(lua_State* vm)
{
	OOLUA_C_FUNCTION(OOLUA::maybe_null<ConVar*>, Lua_Console_FindCvar, const char* )
}

OOLUA_CFUNC(Lua_Console_ExecuteString, LLua_Console_ExecuteString)

//---------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------

bool LuaBinding_InitEngineBindings(lua_State* state)
{
	DebugMessages_InitBinding(state);

	LUA_SET_GLOBAL_ENUMCONST(state, CV_UNREGISTERED);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_CHEAT);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_INITONLY);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_INVISIBLE);
	LUA_SET_GLOBAL_ENUMCONST(state, CV_ARCHIVE);

	LUA_SET_GLOBAL_ENUMCONST(state, SP_ROOT);
	LUA_SET_GLOBAL_ENUMCONST(state, SP_MOD);

	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_STYLE_REGULAR);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_STYLE_BOLD);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_STYLE_ITALIC);

	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_ALIGN_LEFT);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_ALIGN_RIGHT);
	LUA_SET_GLOBAL_ENUMCONST(state, TEXT_ALIGN_HCENTER);

	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_OCCLUSION);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_ROOM_OCCLUSION);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_FORCE_CACHED);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_FORCE_2D);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_STARTSILENT);
	LUA_SET_GLOBAL_ENUMCONST(state, EMITSOUND_FLAG_START_ON_UPDATE);

	OOLUA::register_class<ConVar>(state);
	OOLUA::register_class<Vector2D>(state);
	OOLUA::register_class<Vector3D>(state);
	OOLUA::register_class<Vector4D>(state);
	OOLUA::register_class<Plane>(state);
	OOLUA::register_class<IDebugOverlay>(state);

	OOLUA::register_class<kvkeybase_t>(state);
	OOLUA::register_class<KeyValues>(state);

	OOLUA::register_class<ISoundController>(state);
	OOLUA::register_class<EmitParams>(state);
	OOLUA::register_class<CSoundEmitterSystem>(state);

	OOLUA::register_class<ILocToken>(state);
	OOLUA::register_class<IEqFont>(state);
	OOLUA::register_class<CEqFontCache>(state);

	OOLUA::register_class<Networking::CNetMessageBuffer>(state);
	OOLUA::register_class<Networking::CNetworkThread>(state);

	OOLUA::set_global(state, "LocalizedToken", L_LocalizedToken);
	OOLUA::set_global(state, "AddLanguageFile", L_AddLanguageFile);

	OOLUA::set_global(state, "KV_GetValueString", L_KV_GetValueString);
	OOLUA::set_global(state, "KV_GetValueInt", L_KV_GetValueInt);
	OOLUA::set_global(state, "KV_GetValueFloat", L_KV_GetValueFloat);
	OOLUA::set_global(state, "KV_GetValueBool", L_KV_GetValueBool);
	OOLUA::set_global(state, "KV_GetVector2D", L_KV_GetVector2D);
	OOLUA::set_global(state, "KV_GetVector3D", L_KV_GetVector3D);
	OOLUA::set_global(state, "KV_GetVector4D", L_KV_GetVector4D);

	OOLUA::set_global(state, "f_fract", L_fract);
	OOLUA::set_global(state, "f_lerp", L_lerp);
	OOLUA::set_global(state, "f_cerp", L_cerp);
	OOLUA::set_global(state, "f_sign", L_sign);
	OOLUA::set_global(state, "f_clamp", L_clamp);
	OOLUA::set_global(state, "v2d_lerp", L_v2d_lerp);
	OOLUA::set_global(state, "v2d_cerp", L_v2d_cerp);
	OOLUA::set_global(state, "v2d_sign", L_v2d_sign);
	OOLUA::set_global(state, "v2d_clamp", L_v2d_clamp);

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

	OOLUA::set_global(state, "ConstrainAngle180", L_ConstrainAngle180);
	OOLUA::set_global(state, "ConstrainAngle360", L_ConstrainAngle360);

	OOLUA::set_global(state, "NormalizeAngles180", L_NormalizeAngles180);
	OOLUA::set_global(state, "NormalizeAngles360", L_NormalizeAngles360);

	OOLUA::set_global(state, "AngleDiff", L_AngleDiff);
	OOLUA::set_global(state, "AnglesDiff", L_AnglesDiff);

	OOLUA::set_global(state, "VectorAngles", L_VectorAngles);

	OOLUA::set_global(state, "debugoverlay", debugoverlay);
	OOLUA::set_global(state, "sounds", ses);

	OOLUA::Table consoleTab = OOLUA::new_table(state);
	consoleTab.set("FindCvar", LLua_Console_FindCvar);
	consoleTab.set("ExecuteString", LLua_Console_ExecuteString);

	OOLUA::set_global(state, "console", consoleTab);

	return true;
}
