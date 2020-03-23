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
#ifndef LUABINDING_ENGINE
#define LUABINDING_ENGINE

#include "LuaBinding.h"

#include "LuaBinding_Math.h"

#include "IDebugOverlay.h"
#include "GameSoundEmitterSystem.h"
#include "utils/KeyValues.h"
#include "utils/strtools.h"
#include "FontCache.h"
#include "network/NetMessageBuffer.h"
#include "ILocalize.h"
#include "IConCommandFactory.h"
#include "ICmdLineParser.h"

#include "EqUI/IEqUI_Control.h"
#include "EqUI/EqUI_Manager.h"
#include "EqUI/EqUI_Panel.h"

//#ifndef __INTELLISENSE__

//
// Console command/variable base
//
OOLUA_PROXY( ConCommandBase )

	OOLUA_TAGS(
		No_default_constructor,
		Abstract
	)

	// Names, descs, flags
	OOLUA_MFUNC_CONST(GetName)
	OOLUA_MFUNC_CONST(GetDesc)
	OOLUA_MFUNC_CONST(GetFlags)

	OOLUA_MFUNC_CONST(IsConVar)
	OOLUA_MFUNC_CONST(IsConCommand)
	OOLUA_MFUNC_CONST(IsRegistered)

OOLUA_PROXY_END

//
// Console command
//
OOLUA_PROXY( ConCommand, ConCommandBase )

	OOLUA_TAGS(
		No_default_constructor,
		Abstract
	)

OOLUA_PROXY_END

//
// Console variable
//
OOLUA_PROXY( ConVar, ConCommandBase )

	OOLUA_TAGS(
		No_default_constructor,
		Abstract
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

OOLUA_PROXY_END

OOLUA_PROXY(ICommandLineParse)
	OOLUA_TAGS(
		No_default_constructor,
		Abstract
	)

	OOLUA_MFUNC_CONST(FindArgument)

	OOLUA_MFUNC_CONST(GetArgumentString)

	OOLUA_MFUNC_CONST(GetArgumentsOf)
	OOLUA_MFUNC_CONST(GetArgumentCount)
OOLUA_PROXY_END

//----------------------------------------------

//
// filesystem & streams
//

OOLUA_PROXY(IFile)
	OOLUA_TAGS(
		No_default_constructor,
		Abstract
	)

	// File operations
	OOLUA_MFUNC(Seek)
	OOLUA_MFUNC(Tell)
	OOLUA_MFUNC(GetSize)

	OOLUA_MFUNC(Flush)
	OOLUA_MFUNC(GetCRC32)

OOLUA_PROXY_END

OOLUA_PROXY(IFileSystem)
	OOLUA_TAGS(
		No_default_constructor,
		Abstract
	)

	OOLUA_MFUNC(AddPackage)
	OOLUA_MFUNC(RemovePackage)
	
	OOLUA_MFUNC(AddSearchPath)
	OOLUA_MFUNC(RemoveSearchPath)

	OOLUA_MFUNC_CONST(GetCurrentGameDirectory)
	OOLUA_MFUNC_CONST(GetCurrentDataDirectory)
	
	// File operations
	OOLUA_MEM_FUNC(maybe_null<IFile*>, Open, const char*, const char*, int)
	OOLUA_MFUNC(Close)

	OOLUA_MFUNC_CONST(FileExist)
	OOLUA_MFUNC_CONST(FileRemove)
	OOLUA_MFUNC(FileCopy)
	
	OOLUA_MFUNC(GetFileSize)
	OOLUA_MFUNC(GetFileCRC32)
	//OOLUA_MFUNC(GetFileBuffer)

	OOLUA_MFUNC_CONST(MakeDir)
	OOLUA_MFUNC_CONST(RemoveDir)

OOLUA_PROXY_END

OOLUA_PROXY(CFileSystemFind)
	OOLUA_TAGS(No_default_constructor)

	OOLUA_CTORS(
		OOLUA_CTOR(const char*, int)
	)

	OOLUA_MFUNC(Init)
	OOLUA_MFUNC_CONST(IsDirectory)
	OOLUA_MFUNC_CONST(GetPath)
	OOLUA_MFUNC(Next);

OOLUA_PROXY_END

//----------------------------------------------

//
// keyvalues
//

OOLUA_PROXY(kvpairvalue_t)

	OOLUA_TAGS(
		No_default_constructor,
		Abstract
	)

	OOLUA_MGET(type)
	//OOLUA_MSET(type)

	OOLUA_MGET(value)

	OOLUA_MGET(nValue)
	//OOLUA_MSET(nValue)

	OOLUA_MGET(bValue)
	//OOLUA_MSET(bValue)

	OOLUA_MGET(fValue)
	//OOLUA_MSET(fValue)

	OOLUA_MFUNC(SetValueFrom)

	OOLUA_MFUNC(SetStringValue)
	OOLUA_MFUNC(SetValueFromString)
OOLUA_PROXY_END

OOLUA_PROXY(kvkeybase_t)

	OOLUA_MFUNC(Cleanup)
	OOLUA_MFUNC(ClearValues)

	OOLUA_MFUNC(SetName)
	OOLUA_MFUNC_CONST(GetName)

	OOLUA_MFUNC(AddKeyBase)
	OOLUA_MEM_FUNC(void, AddExistingKeyBase, cpp_in_p<kvkeybase_t*>)
	OOLUA_MFUNC(RemoveKeyBaseByName)
	OOLUA_MEM_FUNC(void, RemoveKeyBase, cpp_in_p<kvkeybase_t*>)

	OOLUA_MEM_FUNC_RENAME(SetKeyString, kvkeybase_t&, SetKey, const char*, const char*)
	OOLUA_MEM_FUNC_RENAME(SetKeyInt, kvkeybase_t&, SetKey, const char*, int)
	OOLUA_MEM_FUNC_RENAME(SetKeyFloat, kvkeybase_t&, SetKey, const char*, float)
	OOLUA_MEM_FUNC_RENAME(SetKeyBool, kvkeybase_t&, SetKey, const char*, bool)
	OOLUA_MEM_FUNC_RENAME(SetKeySection, kvkeybase_t&, SetKey, const char*, kvkeybase_t*)

	OOLUA_MEM_FUNC_RENAME(AddKeyString, kvkeybase_t&, AddKey, const char*, const char*)
	OOLUA_MEM_FUNC_RENAME(AddKeyInt, kvkeybase_t&, AddKey, const char*, int)
	OOLUA_MEM_FUNC_RENAME(AddKeyFloat, kvkeybase_t&, AddKey, const char*, float)
	OOLUA_MEM_FUNC_RENAME(AddKeyBool, kvkeybase_t&, AddKey, const char*, bool)
	OOLUA_MEM_FUNC_RENAME(AddKeySection, kvkeybase_t&, AddKey, const char*, kvkeybase_t*)

	OOLUA_MEM_FUNC_RENAME(AddValueString, void, AddValue, const char*)
	OOLUA_MEM_FUNC_RENAME(AddValueInt, void, AddValue, int)
	OOLUA_MEM_FUNC_RENAME(AddValueFloat, void, AddValue, float)
	OOLUA_MEM_FUNC_RENAME(AddValueBool, void, AddValue, bool)
	OOLUA_MEM_FUNC_RENAME(AddValueSection, void, AddValue, kvkeybase_t*)
	//OOLUA_MEM_FUNC_RENAME(AddValuePair, void, AddValue, kvpairvalue_t*)

	OOLUA_MEM_FUNC_RENAME(AddUniqueValueString, void, AddUniqueValue, const char*)
	OOLUA_MEM_FUNC_RENAME(AddUniqueValueInt, void, AddUniqueValue, int)
	OOLUA_MEM_FUNC_RENAME(AddUniqueValueFloat, void, AddUniqueValue, float)
	OOLUA_MEM_FUNC_RENAME(AddUniqueValueBool, void, AddUniqueValue, bool)

	OOLUA_MEM_FUNC_RENAME(SetValueStringAt, void, SetValueAt, const char*, int)
	OOLUA_MEM_FUNC_RENAME(SetValueIntAt, void, SetValueAt, int, int)
	OOLUA_MEM_FUNC_RENAME(SetValueFloatAt, void, SetValueAt, float, int)
	OOLUA_MEM_FUNC_RENAME(SetValueBoolAt, void, SetValueAt, bool, int)
	//OOLUA_MEM_FUNC_RENAME(SetValuePairAt, void, SetValueAt, kvpairvalue_t*, int)

	OOLUA_MEM_FUNC(void, MergeFrom, const kvkeybase_t*, bool)
	OOLUA_MEM_FUNC_CONST(lua_return<kvkeybase_t*>, Clone)
	OOLUA_MEM_FUNC_CONST(void, CopyTo, kvkeybase_t*)

	OOLUA_MEM_FUNC_CONST(maybe_null<kvkeybase_t*>, FindKeyBase, const char*, int)

	OOLUA_MFUNC_CONST(IsSection)
	OOLUA_MFUNC_CONST(IsArray)
	OOLUA_MFUNC_CONST(IsDefinition)

	OOLUA_MFUNC_CONST(KeyCount)
	OOLUA_MFUNC_CONST(KeyAt)
	OOLUA_MFUNC_CONST(ValueCount)
	OOLUA_MFUNC_CONST(ValueAt)

	OOLUA_MEM_FUNC_CONST_RENAME(GetType, int, L_GetType)
	OOLUA_MEM_FUNC_RENAME(SetType, void, L_SetType, int)

OOLUA_PROXY_END

//-------------------------------

OOLUA_PROXY(KeyValues)

	// loads from file
	OOLUA_MEM_FUNC_RENAME(LoadFile, bool, LoadFromFile, const char*, int)

	OOLUA_MEM_FUNC_RENAME(SaveFile, void, SaveToFile, const char*, int)

	OOLUA_MEM_FUNC_RENAME(GetRoot, kvkeybase_t*, GetRootSection)

	OOLUA_MFUNC(Reset)

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
	OOLUA_MFUNC( Sphere3D )
	OOLUA_MFUNC( Polygon3D )
OOLUA_PROXY_END

//----------------------------------------------

//
// Sound controller
//
OOLUA_PROXY( ISoundController )
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC_CONST( IsStopped )

	OOLUA_MFUNC( StartSound )

	OOLUA_MFUNC( Play )
	OOLUA_MFUNC( Pause )
	OOLUA_MFUNC( Stop )

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

	OOLUA_MGET_MSET(sampleId)
OOLUA_PROXY_END

//
// Sound emitter system
//
OOLUA_PROXY( CSoundEmitterSystem )
	OOLUA_TAGS( Abstract )

	OOLUA_MEM_FUNC_RENAME( Precache, void, PrecacheSound, const char* )
	OOLUA_MEM_FUNC_RENAME( Emit, int, EmitSound, EmitParams* )
	OOLUA_MEM_FUNC_RENAME( Emit2D, void, Emit2DSound, EmitParams*, int )

	OOLUA_MEM_FUNC_RENAME( LoadScript, void, LoadScriptSoundFile, const char* )

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

//
// UI panel
//

OOLUA_PROXY(equi::IUIControl)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC( InitFromKeyValues )

	OOLUA_MFUNC_CONST( GetName )
	OOLUA_MFUNC( SetName )

	OOLUA_MFUNC_CONST( GetLabel )
	OOLUA_MFUNC( SetLabel )

	OOLUA_MFUNC( Show )
	OOLUA_MFUNC( Hide )

	OOLUA_MFUNC( SetVisible )
	OOLUA_MFUNC_CONST( IsVisible )

	OOLUA_MFUNC( SetSelfVisible )
	OOLUA_MFUNC_CONST( IsSelfVisible )

	OOLUA_MFUNC( Enable )
	OOLUA_MFUNC_CONST( IsEnabled )

	OOLUA_MFUNC( SetSize )
	OOLUA_MFUNC( SetPosition )

	OOLUA_MFUNC_CONST( GetSize )
	OOLUA_MFUNC_CONST( GetPosition )

	//OOLUA_MFUNC( SetRectangle )
	//OOLUA_MFUNC_CONST( GetRectangle )

	//OOLUA_MFUNC_CONST( GetClientRectangle )

	// child controls
	OOLUA_MFUNC( AddChild )
	OOLUA_MFUNC( RemoveChild )
	OOLUA_MEM_FUNC( maybe_null<equi::IUIControl*>, FindChild, const char* )
	OOLUA_MFUNC( ClearChilds )

	OOLUA_MEM_FUNC_CONST( maybe_null<equi::IUIControl*>, GetParent )

	//OOLUA_MFUNC_CONST( GetFont )
	OOLUA_MFUNC_CONST( GetClassname )

OOLUA_PROXY_END

OOLUA_PROXY(equi::Panel, equi::IUIControl)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC( SetColor )
	OOLUA_MFUNC_CONST( GetColor )

	OOLUA_MFUNC( SetSelectionColor )
	OOLUA_MFUNC_CONST( GetSelectionColor )

	OOLUA_MFUNC( CenterOnScreen )
OOLUA_PROXY_END

//
// UI panel manager
//

OOLUA_PROXY(equi::CUIManager)
	OOLUA_TAGS( Abstract )

	OOLUA_MEM_FUNC_CONST( maybe_null<equi::Panel*>, GetRootPanel )

	//OOLUA_MFUNC( RegisterFactory )

	OOLUA_MEM_FUNC( maybe_null<equi::IUIControl*>, CreateElement, const char* )

	OOLUA_MFUNC( AddPanel )
	OOLUA_MFUNC( DestroyPanel )
	OOLUA_MEM_FUNC_CONST( maybe_null<equi::Panel*>, FindPanel, const char* )

	OOLUA_MFUNC( BringToTop )
	OOLUA_MEM_FUNC_CONST( maybe_null<equi::Panel*>, GetTopPanel )

	//OOLUA_MFUNC( SetViewFrame )
	//OOLUA_MFUNC_CONST( GetViewFrame )
	OOLUA_MFUNC_CONST(GetScreenSize)

	OOLUA_MFUNC( SetFocus )
	OOLUA_MEM_FUNC_CONST( maybe_null<equi::IUIControl*>, GetFocus )
	OOLUA_MFUNC_CONST( GetMouseOver )

	OOLUA_MFUNC_CONST( IsWindowsVisible )

	OOLUA_MFUNC_CONST( GetDefaultFont )
OOLUA_PROXY_END

//#endif // __INTELLISENSE__

#define LUA_SET_GLOBAL_ENUMCONST(state, constName)	\
		auto l_##constName = constName;			\
		OOLUA::set_global(state, #constName, l_##constName)

//----------------------------------------------------------------------------------------

bool LuaBinding_InitEngineBindings(lua_State* state);
void LuaBinding_ShutdownEngineBindings();
bool LuaBinding_ConsoleHandler(const char* cmdText);

#endif // LUABINDING_ENGINE
