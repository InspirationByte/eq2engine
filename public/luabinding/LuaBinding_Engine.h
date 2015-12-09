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
#ifndef LUABINDING_ENGINE
#define LUABINDING_ENGINE

#include "LuaBinding.h"

#include "LuaBinding_Math.h"
#include "IDebugOverlay.h"
#include "GameSoundEmitterSystem.h"
#include "utils/KeyValues.h"
#include "FontCache.h"
#include "network/NetMessageBuffer.h"

#ifndef __INTELLISENSE__

//
// Console variable
//
OOLUA_PROXY( ConVar )

	OOLUA_TAGS(
		No_default_constructor
	)

	OOLUA_CTORS(
		OOLUA_CTOR(char const *,char const *,char const *, int)
	)

	OOLUA_MFUNC(RevertToDefaultValue)

	OOLUA_MEM_FUNC_RENAME(SetString, void, SetValue, const char*)
	OOLUA_MFUNC(SetFloat)
	OOLUA_MFUNC(SetInt)
	OOLUA_MFUNC(SetBool)

	OOLUA_MFUNC_CONST(HasClamp)
	OOLUA_MFUNC_CONST(GetMinClamp)
	OOLUA_MFUNC_CONST(GetMaxClamp)

	// Value returning
	OOLUA_MFUNC_CONST(GetFloat)
	OOLUA_MFUNC_CONST(GetString)
	OOLUA_MFUNC_CONST(GetInt)
	OOLUA_MFUNC_CONST(GetBool)

	OOLUA_MFUNC_CONST(GetName)

OOLUA_PROXY_END


//----------------------------------------------

//
// keyvalues
//

OOLUA_PROXY(kvkeybase_t)

	OOLUA_MFUNC(Cleanup)
	OOLUA_MFUNC(ClearValues)
	OOLUA_MFUNC(SetName)
	
	
	OOLUA_MFUNC(AddKeyBase)
	OOLUA_MFUNC(SetKey)
	OOLUA_MFUNC(RemoveKeyBase)
	
	OOLUA_MFUNC(SetValue)
	OOLUA_MFUNC(SetValueByIndex)
	OOLUA_MFUNC(AppendValue)
	OOLUA_MEM_FUNC(void, MergeFrom, const kvkeybase_t*, bool)

	OOLUA_MEM_FUNC_CONST(maybe_null<kvkeybase_t*>, FindKeyBase, const char*, int)

	OOLUA_MFUNC_CONST(IsSection)
	OOLUA_MFUNC_CONST(IsArray)
	OOLUA_MFUNC_CONST(IsDefinition)

OOLUA_PROXY_END

//-------------------------------

OOLUA_PROXY(KeyValues)

	// loads from file
	OOLUA_MEM_FUNC_RENAME(LoadFile, bool, LoadFromFile, const char*, int)

	OOLUA_MEM_FUNC_RENAME(SaveFile, void, SaveToFile, const char*, int)

	OOLUA_MEM_FUNC_RENAME(GetRoot, kvkeybase_t*, GetRootSection)

OOLUA_PROXY_END


//----------------------------------------------

//
// debugoverlay
//

OOLUA_PROXY( IDebugOverlay )
	OOLUA_TAGS( Abstract )		// abstract to deny creation

	//OOLUA_MFUNC( Text)
	//OOLUA_MFUNC( TextFadeOut )
	//OOLUA_MFUNC( Text3D )

	OOLUA_MFUNC( Line3D )
	OOLUA_MFUNC( Box3D )
	OOLUA_MFUNC( Polygon3D )
OOLUA_PROXY_END

//----------------------------------------------

//
// Sound controller
//
OOLUA_PROXY( ISoundController )
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC_CONST( IsStopped )

	OOLUA_MEM_FUNC_RENAME( Start, void, StartSound, const char* )
	OOLUA_MEM_FUNC_RENAME( Pause, void, PauseSound )
	OOLUA_MEM_FUNC_RENAME( Stop, void, StopSound )

	OOLUA_MFUNC( SetPitch )
	OOLUA_MFUNC( SetVolume )
	OOLUA_MFUNC( SetOrigin )
	OOLUA_MFUNC( SetVelocity )
OOLUA_PROXY_END

//
// Emitsound params
//
OOLUA_PROXY( EmitParams )
	OOLUA_TAGS( No_default_constructor )

	OOLUA_CTORS(
		OOLUA_CTOR( const char* )
		OOLUA_CTOR( const char*, int)
		OOLUA_CTOR( const char*, float ,float )
		OOLUA_CTOR( const char*, const Vector3D& )
		OOLUA_CTOR( const char*, const Vector3D&, float, float )
	)
OOLUA_PROXY_END

//
// Sound emitter system
//
OOLUA_PROXY( CSoundEmitterSystem )
	OOLUA_TAGS( Abstract )

	OOLUA_MEM_FUNC_RENAME( Precache, void, PrecacheSound, const char* )
	OOLUA_MEM_FUNC_RENAME( Emit, int, EmitSound, EmitParams* )
	OOLUA_MEM_FUNC_RENAME( Emit2D, void, Emit2DSound, EmitParams*, int )

	OOLUA_MEM_FUNC_RENAME( CreateController, ISoundController*, CreateSoundController, EmitParams* )
	OOLUA_MEM_FUNC_RENAME( RemoveController, void, RemoveSoundController, ISoundController* )

OOLUA_PROXY_END

//
// Localizer
//

OOLUA_PROXY( ILocToken )
	OOLUA_TAGS( Abstract )
OOLUA_PROXY_END

//
// Font system
//

OOLUA_PROXY( IEqFont )
	OOLUA_TAGS( Abstract )
OOLUA_PROXY_END

OOLUA_PROXY( CEqFontCache )
	OOLUA_TAGS( Abstract )
	OOLUA_MFUNC_CONST( GetFont )
OOLUA_PROXY_END

#endif // __INTELLISENSE__

#define LUA_SET_GLOBAL_ENUMCONST(state, constName)	\
		auto l_##constName = constName;			\
		OOLUA::set_global(state, #constName, l_##constName)

//----------------------------------------------------------------------------------------

bool LuaBinding_InitEngineBindings(lua_State* state);

#endif // LUABINDING_ENGINE