//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Title screen
//////////////////////////////////////////////////////////////////////////////////

#include "state_mainmenu.h"
#include "state_game.h"

#include "in_keys_ident.h"
#include "system.h"
#include "FontCache.h"

#include "KeyBinding/Keys.h"

enum
{
	MENU_ENTER = 1,
	MENU_BACK
};

CState_MainMenu::CState_MainMenu()
{
	m_titleTexture = NULL;
}

CState_MainMenu::~CState_MainMenu()
{

}

void CState_MainMenu::OnEnter( CBaseStateHandler* from )
{
	ses->Init();

	m_fade = 0.0f;
	m_textFade = 0.0f;
	m_changesMenu = 0;
	m_goesFromMenu = false;
	m_titleTexture = g_pShaderAPI->LoadTexture("ui/title", TEXFILTER_TRILINEAR_ANISO, ADDRESSMODE_CLAMP);
	m_titleTexture->Ref_Grab();
	g_pHost->SetCursorShow(false);

	soundsystem->SetPauseState(false);
	soundsystem->SetVolumeScale(1.0f);
	sndEffect_t* reverb = soundsystem->FindEffect( "menu_reverb" );

	soundsystem->SetListener(vec3_zero, vec3_forward, vec3_up, vec3_zero, reverb);

	//-------------------------

	OOLUA::Table mainMenuStack;
	if(!OOLUA::get_global(GetLuaState(), "MainMenuStack", mainMenuStack))
	{
		WarningMsg("Failed to get MainMenuStack table (DrvSynMenus.lua ???)!\n");
	}

	SetMenuObject( mainMenuStack );

	//EmitSound_t es("music.menu", EMITSOUND_FLAG_FORCE_CACHED);
	//ses->Emit2DSound( &es );

	soundsystem->SetVolumeScale(0.0f);
}
/*
void CState_MainMenu::SetMenuObject(OOLUA::Table& tabl)
{
	m_menuStack = tabl;

	lua_State* state = GetLuaState();

	m_stackReset.Get(state, tabl, "Reset");
	m_stackGetCurMenu.Get(state, tabl, "GetCurrentMenu");
	m_stackGetTop.Get(state, tabl, "GetTop");
	m_stackPush.Get(state, tabl, "Push");
	m_stackPop.Get(state, tabl, "Pop");
	m_stackCanPop.Get(state, tabl, "CanPop");

	if( m_stackGetTop.Push(state) && m_stackGetTop.Call(state, 0, 1) )
	{
		OOLUA::Table stackTop;
		OOLUA::pull( state, stackTop );

		SetMenuStackTop( stackTop );
	}
}

void CState_MainMenu::SetMenuStackTop(OOLUA::Table& tabl)
{
	lua_State* state = GetLuaState();

	if( m_stackGetCurMenu.Push(state) && m_stackGetCurMenu.Call(state, 0, 1) )
	{
		OOLUA::Table menuTable;
		OOLUA::pull( state, menuTable );

		SetMenuTable( menuTable );
	}
}

void CState_MainMenu::PushMenu(OOLUA::Table& tabl)
{
	lua_State* state = GetLuaState();

	if( m_stackPush.Push(state) && OOLUA::push(state, tabl) && m_stackPush.Call(state, 1, 1) )
	{
		OOLUA::Table newStackTop;
		OOLUA::pull( state, newStackTop );

		SetMenuStackTop( newStackTop );
	}
}

void CState_MainMenu::PopMenu()
{
	lua_State* state = GetLuaState();

	if( m_stackPop.Push(state) && m_stackPop.Call(state, 0, 1) )
	{
		OOLUA::Table newStackTop;
		OOLUA::pull( state, newStackTop );

		SetMenuStackTop( newStackTop );
	}
}

bool CState_MainMenu::IsCanPopMenu()
{
	lua_State* state = GetLuaState();

	if( m_stackCanPop.Push(state) && m_stackCanPop.Call(state, 0, 1) )
	{
		bool canPop = false;
		OOLUA::pull( state, canPop );

		return canPop;
	}

	return false;
}

void CState_MainMenu::PreEnterSelection()
{
	OOLUA::Table elem;
	if( m_menuElems.safe_at(m_selection+1, elem) )
	{
		bool isFinal = false;
		elem.safe_at("isFinal", isFinal);

		m_goesFromMenu = isFinal;
	}
}

void CState_MainMenu::EnterSelection()
{
	lua_State* state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	OOLUA::Table elem;
	if( m_menuElems.safe_at(m_selection+1, elem) )
	{
		EqLua::LuaTableFuncRef onEnter;
		if(onEnter.Get(state, elem, "onEnter", false) && onEnter.Push(state) && onEnter.Call(state, 1, 1))
		{
			if( onEnter.Push(state) && onEnter.Call(state, 0, 1) )
			{
				if( lua_type(state, -1) == LUA_TTABLE )
				{
					OOLUA::Table newMenu;
					OOLUA::pull( state, newMenu );

					if(!m_goesFromMenu)
						PushMenu( newMenu );
				}
			}
		}
	}
}

void CState_MainMenu::SetMenuTable( OOLUA::Table& tabl )
{
	m_menuElems = tabl;

	m_selection = 0;
	m_numElems = 0;

	EqLua::LuaStackGuard g(GetLuaState());

	oolua_ipairs(m_menuElems)
		m_numElems++;
	oolua_ipairs_end()
}*/

void CState_MainMenu::OnEnterSelection( bool isFinal )
{
	m_goesFromMenu = isFinal;
}

void CState_MainMenu::OnLeave( CBaseStateHandler* to )
{
	ses->Shutdown();

	g_pShaderAPI->FreeTexture(m_titleTexture);
	m_titleTexture = NULL;
}

bool CState_MainMenu::Update( float fDt )
{
	if( m_goesFromMenu )
	{
		m_fade -= fDt;
		m_textFade -= fDt*4.0f;
	}
	else
	{
		m_fade += fDt;

		if(m_changesMenu > 0)
			m_textFade -= fDt*2.0f;
		else
			m_textFade += fDt*4.0f;
	}

	m_fade = clamp(m_fade, 0.0f, 1.0f);
	m_textFade = clamp(m_textFade, 0.0f, 1.0f);

	materials->Setup2D(g_pHost->m_nWidth, g_pHost->m_nHeight);
	g_pShaderAPI->Clear( true,true, false, ColorRGBA(0.25f,0,0,0.0f));

	IVector2D screen(g_pHost->m_nWidth, g_pHost->m_nHeight);
	IVector2D halfScreen(g_pHost->m_nWidth/2, g_pHost->m_nHeight/2);

	float widthFac = (float)g_pHost->m_nWidth/(float)m_titleTexture->GetWidth();
	float heightFac = (float)g_pHost->m_nHeight/(float)m_titleTexture->GetHeight();
	float texSizeFactor = widthFac > heightFac ? widthFac : heightFac;

	float halfWidth = m_titleTexture->GetWidth()*0.5f;
	float halfHeight = m_titleTexture->GetHeight()*0.5f;

	Vertex2D_t tmprect[] = { 
		MAKETEXQUAD((halfScreen.x-halfWidth*texSizeFactor), 
					(halfScreen.y-halfHeight*texSizeFactor), 
					(halfScreen.x+halfWidth*texSizeFactor), 
					(halfScreen.y+halfHeight*texSizeFactor), 0.0f) 
		};

	// Cancel textures
	g_pShaderAPI->Reset(STATE_RESET_TEX);

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	float fFade = pow(m_fade,2.0f);

	soundsystem->SetVolumeScale(fFade);

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), m_titleTexture, ColorRGBA(fFade,fFade,fFade,1.0f), &blending);

	IEqFont* font = g_fontCache->GetFont("Roboto", 40, TEXT_STYLE_ITALIC);

	eqFontStyleParam_t fontParam;
	fontParam.align = TEXT_ALIGN_LEFT;
	fontParam.styleFlag |= TEXT_STYLE_SHADOW;
	fontParam.textColor = color4_white;

	{
		EqLua::LuaStackGuard g(GetLuaState());

		int menuPosY = halfScreen.y+100;

		oolua_ipairs(m_menuElems)
			int idx = _i_index_-1;

			OOLUA::Table elem;
			m_menuElems.safe_at(_i_index_, elem);

			ILocToken* tok = NULL;
			elem.safe_at("label", tok);

			if(m_selection == idx)
				fontParam.textColor = ColorRGBA(1,0.7f,0.0f,pow(m_textFade*m_fade,5.0f));
			else
				fontParam.textColor = ColorRGBA(1,1,1,pow(m_textFade*m_fade,5.0f));

			if((m_goesFromMenu || m_changesMenu == MENU_ENTER) && m_changesMenu != MENU_BACK)
			{
				if(m_selection == idx)
				{
					float sqAlphaValue = clamp(sin(m_textEffect*32.0f)*16,0.0f,1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

					if(m_textEffect > 0.75f)
						fontParam.textColor.w = sqAlphaValue;
					else
					{
						float fadeVal = (m_textEffect-0.25f) / 0.75f;
						if(fadeVal < 0)
							fadeVal = 0;

						fontParam.textColor.w = fadeVal;
					}
				}
				else
				{
					fontParam.textColor.w = pow(m_textFade*m_fade,10.0f);
				}
			}

			Vector2D elemPos(halfScreen.x-500, menuPosY+idx*45);

			font->RenderText(tok ? tok->GetText() : L"Undefined token", elemPos, fontParam);
		oolua_ipairs_end()
	}

	m_textEffect -= fDt;

	if(m_textEffect < 0)
		m_textEffect = 0.0f;

	if(m_changesMenu > 0 && m_textFade <= 0.0f)
	{
		if(m_changesMenu == MENU_ENTER)
		{
			EnterSelection();
		}
		else if(m_changesMenu == MENU_BACK)
		{
			PopMenu();
		}

		m_changesMenu = 0;
	}

	return !(m_goesFromMenu && m_fade == 0.0f);
}

void CState_MainMenu::HandleKeyPress( int key, bool down )
{
	if(m_changesMenu > 0)
		return;

	if(m_goesFromMenu)
		return;

	if(key == KEY_ENTER && down == true && m_fade == 1.0f)
	{
		EmitSound_t es("menu.click", EMITSOUND_FLAG_FORCE_CACHED);
		ses->EmitSound( &es );

		m_changesMenu = MENU_ENTER;
		m_textEffect = 1.0f;
		m_textFade = 1.0f;

		PreEnterSelection();
	}
	else if(down)
	{
		if(key == KEY_ESCAPE)
		{
			EmitSound_t es("menu.back", EMITSOUND_FLAG_FORCE_CACHED);
			ses->EmitSound( &es );

			if(IsCanPopMenu())
			{
				m_changesMenu = MENU_BACK;
				m_textEffect = 1.0f;
				m_textFade = 1.0f;
			}
			else
			{
				m_changesMenu = MENU_BACK;
				m_goesFromMenu = true;
				//m_textEffect = 1.0f;
				//m_textFade = 1.0f;
				SetNextState(g_states[GAME_STATE_TITLESCREEN]);
			}
		}
		else if(key == KEY_UP)
		{
	redecrement:

			m_selection--;

			if(m_selection < 0)
			{
				int nItem = 0;
				m_selection = m_numElems-1;
			}

			//if(pItem->type == MIT_SPACER)
			//	goto redecrement;

			EmitSound_t ep("menu.roll", EMITSOUND_FLAG_FORCE_CACHED);
			ses->EmitSound(&ep);
		}
		else if(key == KEY_DOWN)
		{
	reincrement:
			m_selection++;

			if(m_selection > m_numElems-1)
				m_selection = 0;

			//if(pItem->type == MIT_SPACER)
			//	goto reincrement;

			EmitSound_t ep("menu.roll", EMITSOUND_FLAG_FORCE_CACHED);
			ses->EmitSound(&ep);
		}
	}

	GetKeyBindings()->OnKeyEvent( key, down );
}

CState_MainMenu* g_State_MainMenu = new CState_MainMenu();