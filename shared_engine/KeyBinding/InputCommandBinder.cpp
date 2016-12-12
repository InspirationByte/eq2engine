//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: KeyBinding list
//////////////////////////////////////////////////////////////////////////////////

#include "InputCommandBinder.h"

#include "IConCommandFactory.h"
#include "DebugInterface.h"
#include "utils/strtools.h"
#include "InterfaceManager.h"

#include "FontCache.h"

#include "materialsystem/IMaterialSystem.h"
#include "utils/KeyValues.h"

#if !defined(EDITOR) && !defined(NO_ENGINE)
#include "IEngineGame.h"
#include "sys_console.h"
#endif

static CInputCommandBinder s_inputCommandBinder;
CInputCommandBinder* g_inputCommandBinder = &s_inputCommandBinder;

ConVar in_keys_debug("in_keys_debug", "0", "Debug CInputCommandBinder");
ConVar in_touchzones_debug("in_touchzones_debug", "0", "Debug touch zones on screen and messages", CV_CHEAT);

DECLARE_CMD(in_touchzones_reload, "Reload touch zones", 0)
{
	g_inputCommandBinder->InitTouchZones();
}

CInputCommandBinder::CInputCommandBinder()
{
}

void CInputCommandBinder::Init()
{
	InitTouchZones();
}

void CInputCommandBinder::Shutdown()
{
	m_touchZones.clear();

	UnbindAll();
}

void CInputCommandBinder::InitTouchZones()
{
	m_touchZones.clear();

	KeyValues kvs;
	if(!kvs.LoadFromFile("resources/in_touchzones.res"))
		return;

	kvkeybase_t* zones = kvs.GetRootSection()->FindKeyBase("zones");

	for(int i = 0; i < zones->keys.numElem(); i++)
	{
		kvkeybase_t* zoneDef = zones->keys[i];

		touchzone_t newZone;
		newZone.name = zoneDef->name;

		kvkeybase_t* zoneCmd = zoneDef->FindKeyBase("bind");

		newZone.commandString = KV_GetValueString(zoneCmd, 0, "zone_no_bind");
		newZone.argumentString = KV_GetValueString(zoneCmd, 1, "");

		newZone.position = KV_GetVector2D(zoneDef->FindKeyBase("position"));
		newZone.size = KV_GetVector2D(zoneDef->FindKeyBase("size"));

		// resolve commands

		// if we connecting libraries dynamically, that wouldn't properly execute
		newZone.boundCommand1 = (ConCommand*)g_sysConsole->FindCommand(varargs("+%s", newZone.commandString.c_str()));
		newZone.boundCommand2 = (ConCommand*)g_sysConsole->FindCommand(varargs("-%s", newZone.commandString.c_str()));

		// if found only one command with plus or minus
		if(!newZone.boundCommand1 || !newZone.boundCommand2)
			newZone.boundCommand1 = (ConCommand*)g_sysConsole->FindCommand( newZone.commandString.c_str() );

		DevMsg(DEVMSG_CORE, "Touchzone: %s (%s) [x=%g,y=%g] [w=%g,h=%g]\n", 
			newZone.name.c_str(), newZone.commandString.c_str(), 
			newZone.position.x, newZone.position.y, 
			newZone.size.x, newZone.size.y);

		// if anly command found
		if(newZone.boundCommand1 || newZone.boundCommand2)
		{
			m_touchZones.append( newZone );
		}
		else
		{
			MsgError("touchzone %s: unknown command '%s'\n", newZone.name.c_str(), newZone.commandString.c_str());
		}
	}
}

// saves binding using file handle
void CInputCommandBinder::WriteBindings(IFile* cfgFile)
{
	if(!cfgFile)
		return;

	cfgFile->Print("unbindall\n" );

	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		// resolve key name
		EqString keyNameString = s_keynames[ m_pBindings[i]->key_index ].name;

		cfgFile->Print("bind %s %s %s\n",	keyNameString.GetData(),
											m_pBindings[i]->commandString.GetData(),
											m_pBindings[i]->argumentString.GetData() );
	}
}


// binds a command with arguments to known key
void CInputCommandBinder::BindKey( const char* pszCommand, const char *pszArgs, const char* pszKeyStr )
{
	// check if key bound

	// Find the key matching the *keychar
	int keyindex = KeyStringToKeyIndex( pszKeyStr );

	if(keyindex == -1)
	{
		MsgError("Unknown key identifier '%s'\n", pszKeyStr);
		return;
	}

	// create new binding
	binding_t *keybind = new binding_t;

	keybind->key_index = keyindex;

	keybind->commandString = pszCommand;

	if(pszArgs)
		keybind->argumentString = pszArgs;

	// recognize mouse command
	if(_Es(pszKeyStr).Find("mouse") != -1)
		keybind->isMouse = true;

	// resolve commands

	// if we connecting libraries dynamically, that wouldn't properly execute
	keybind->boundCommand1 = (ConCommand*)g_sysConsole->FindCommand(varargs("+%s", pszCommand));
	keybind->boundCommand2 = (ConCommand*)g_sysConsole->FindCommand(varargs("-%s", pszCommand));

	// if found only one command with plus or minus
	if(!keybind->boundCommand1 || !keybind->boundCommand2)
		keybind->boundCommand1 = (ConCommand*)g_sysConsole->FindCommand( pszCommand );

	// if anly command found
	if(keybind->boundCommand1 || keybind->boundCommand2)
	{
		m_pBindings.append( keybind );
	}
	else
	{
		MsgError("Unknown command '%s'\n", pszCommand);
		delete keybind;
	}
}

// returns binding
binding_t* CInputCommandBinder::LookupBinding(uint keyIdent)
{
	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == keyIdent)
			return m_pBindings[i];
	}

	return NULL;
}

// searches for binding
binding_t* CInputCommandBinder::FindBinding(const char* pszKeyStr)
{
	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(!stricmp( s_keynames[m_pBindings[i]->key_index].name, pszKeyStr ))
			return m_pBindings[i];
	}

	return NULL;
}

// removes single binding on specified keychar
void CInputCommandBinder::RemoveBinding(const char* pszKeyStr)
{
	int index = KeyStringToKeyIndex( pszKeyStr );
	if(index == -1)
	{
		Msg("Unknown key identifier '%s'\n", pszKeyStr);
		return;
	}

	bool bResult = false;

	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(m_pBindings[i]->key_index == index)
		{
			delete m_pBindings[i];

			m_pBindings.removeIndex(i);
			i--;

			bResult = true;
		}
	}

	if(bResult)
		MsgInfo("'%s' unbound\n", pszKeyStr);
}

// clears and removes all key bindings
void CInputCommandBinder::UnbindAll()
{
	for(int i = 0; i < m_pBindings.numElem();i++)
		delete m_pBindings[i];

	m_pBindings.clear();
}

void CInputCommandBinder::UnbindAll_Joystick()
{
	for(int i = 0; i < m_pBindings.numElem();i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum >= JOYSTICK_START_KEYS)
		{
			delete m_pBindings[i];
			m_pBindings.removeIndex(i);
			i--;
		}
	}
}

// registers axis action
void CInputCommandBinder::RegisterJoyAxisAction( int axis, const char* name, JOYAXISFUNC axisFunc )
{

}

void CInputCommandBinder::BindJoyAxis( int axis, const char* actionName )
{

}

//
// Event processing
//
void CInputCommandBinder::OnKeyEvent(const int keyIdent, bool bPressed)
{
	if(in_keys_debug.GetBool())
		MsgWarning("-- KeyPress: %s (%d)\n", KeyIndexToString(keyIdent), bPressed);

	for(int i = 0; i < m_pBindings.numElem(); i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == keyIdent)
		{
			ExecuteBinding( m_pBindings[i], bPressed);
		}
	}
}

void CInputCommandBinder::OnMouseEvent( const int button, bool bPressed )
{
	for(int i = 0; i < m_pBindings.numElem(); i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == button)
		{
			ExecuteBinding( m_pBindings[i], bPressed);
		}
	}
}

void CInputCommandBinder::OnMouseWheel( const int scroll )
{
	int button = (scroll > 0) ?  MOU_WHUP : MOU_WHDN;

	for(int i = 0; i < m_pBindings.numElem(); i++)
	{
		if(s_keynames[m_pBindings[i]->key_index].keynum == button)
		{
			ExecuteBinding( m_pBindings[i], (scroll > 0));
		}
	}
}

void CInputCommandBinder::OnTouchEvent( const Vector2D& pos, int finger, bool down )
{
	if(in_touchzones_debug.GetBool())
		MsgWarning("-- Touch [%g %g] (%d)\n", pos.x, pos.y, down);

	for(int i = 0; i < m_touchZones.numElem(); i++)
	{
		touchzone_t* tz = &m_touchZones[i];

		Rectangle_t rect(tz->position - tz->size*0.5f, tz->position + tz->size*0.5f);

		if(!down)
		{
			if(tz->finger == finger) // if finger up
			{
				ExecuteTouchZone( tz, down );
				tz->finger = -1;
			}
		}
		else if( rect.IsInRectangle(pos) )
		{
			if(in_touchzones_debug.GetBool())
				Msg("found zone %s\n", tz->name.c_str());

			tz->finger = finger;

			ExecuteTouchZone( tz, down );
			continue;
		}
	}
}

void CInputCommandBinder::DebugDraw(const Vector2D& screenSize)
{
	if(!in_touchzones_debug.GetBool())
		return;

	materials->Setup2D(screenSize.x,screenSize.y);

	eqFontStyleParam_t fontParams;
	fontParams.styleFlag |= TEXT_STYLE_SHADOW;
	fontParams.textColor = color4_white;

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	static IEqFont* defaultFont = g_fontCache->GetFont("default", 30);

	for(int i = 0; i < m_touchZones.numElem(); i++)
	{
		touchzone_t* tz = &m_touchZones[i];

		Rectangle_t rect((tz->position-tz->size*0.5f)*screenSize, (tz->position+tz->size*0.5f)*screenSize);

		defaultFont->RenderText( tz->name.c_str() , rect.vleftTop, fontParams);

		Vertex2D_t touchQuad[] = { MAKETEXQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, 0) };
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,touchQuad,elementsOf(touchQuad), NULL, ColorRGBA(0.435,0.435,0.435, tz->finger >= 0 ? 0.25f : 0.35f  ), &blending);
	}

}

// executes binding with selected state
void CInputCommandBinder::ExecuteBinding( binding_t* pBinding, bool bState )
{
	ExecuteBoundCommands(pBinding, bState);
}

void CInputCommandBinder::ExecuteTouchZone( touchzone_t* zone, bool bState )
{
	ExecuteBoundCommands(zone, bState);
}

template <typename T>
void CInputCommandBinder::ExecuteBoundCommands(T* zone, bool bState)
{
	DkList<EqString> args;

	xstrsplit( zone->argumentString.GetData(), " ", args);

	ConCommand *cmd = NULL;

	if(bState) //Handle press and de-press
		cmd = zone->boundCommand1;
	else
		cmd = zone->boundCommand2;

	// if there is only one command
	if(!cmd && bState)
		cmd = zone->boundCommand1;

	// dispatch command
	if(cmd)
	{
#if !defined(EDITOR) && !defined(NO_ENGINE)
		// skip client controls if console enabled
		if((cmd->GetFlags() & CV_CLIENTCONTROLS) && (engine->GetGameState() == IEngineGame::GAME_PAUSE || g_pSysConsole->IsVisible()))
			return;
#endif

		if(in_keys_debug.GetBool())
			MsgWarning("dispatch %s\n", cmd->GetName());

		cmd->DispatchFunc( args );
	}
}

#ifndef DLL_EXPORT

void con_key_list(DkList<EqString>& list, const char* query)
{
	const int LIST_LIMIT = 50;

	keyname_t* names = s_keynames;

	do
	{
		keyname_t& name = *names;

		if(name.name == NULL)
			break;

		if(list.numElem() == LIST_LIMIT)
		{
			list.append("...");
			break;
		}

		if(*query == 0 || xstristr(name.name, query))
			list.append(name.name);

	}while(names++);
}

DECLARE_CMD_VARIANTS(bind,"Binds action to key", con_key_list, 0)
{
	if(CMD_ARGC > 1)
	{
		EqString agrstr;

		for(int i = 2; i < CMD_ARGC; i++)
			agrstr.Append(varargs("%s ",CMD_ARGV(i).c_str()));

		g_inputCommandBinder->BindKey(CMD_ARGV(1).c_str(),(char*)agrstr.GetData(), CMD_ARGV(0).c_str());
	}
	else
		MsgInfo("Usage: bind <key> <command> [args,...]\n");
}

DECLARE_CMD(list_binding,"Shows bound keys",0)
{
	MsgInfo("---- List of bound keys to commands ----\n");
	DkList<binding_t*> *bindingList = g_inputCommandBinder->GetBindingList();

	for(int i = 0; i < bindingList->numElem();i++)
	{
		binding_t* bind = bindingList->ptr()[i];

		const char* keyName = s_keynames[bind->key_index].name;

		Msg("bind %s %s %s\n", keyName, bind->commandString.c_str(), bind->argumentString.c_str() );
	}

	MsgInfo("---- %d keys/commands bound ----\n", bindingList->numElem());
}

DECLARE_CMD(list_touchzones,"Shows bound keys",0)
{
	MsgInfo("---- List of bound touchzones to commands ----\n");
	DkList<touchzone_t> *touchList = g_inputCommandBinder->GetTouchZoneList();

	for(int i = 0; i < touchList->numElem();i++)
	{
		touchzone_t* tz = &touchList->ptr()[i];

		Msg("Touchzone %s (%s) [x=%g,y=%g] [w=%g,h=%g]\n", tz->name.c_str(), tz->commandString.c_str(), tz->position.x, tz->position.y, tz->size.x, tz->size.y);
	}

	MsgInfo("---- %d touch zones ----\n", touchList->numElem());
}

DECLARE_CMD_VARIANTS(unbind,"Unbinds a key", con_key_list, 0)
{
	if(CMD_ARGC > 0)
	{
		g_inputCommandBinder->RemoveBinding(CMD_ARGV(0).c_str());
	}
}

DECLARE_CMD(unbindall,"Unbinds all keys",0)
{
	g_inputCommandBinder->UnbindAll();
}

DECLARE_CMD(unbindjoystick,"Unbinds joystick controls",0)
{
	g_inputCommandBinder->UnbindAll_Joystick();
}


#endif // DLL_EXPORT
