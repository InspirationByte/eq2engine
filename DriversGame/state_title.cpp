//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Title screen
//////////////////////////////////////////////////////////////////////////////////

#include "state_mainmenu.h"
#include "state_title.h"
#include "state_game.h"

#include "in_keys_ident.h"
#include "sys_host.h"
#include "FontCache.h"
#include "replay.h"

#include "EqUI/EqUI_Manager.h"

#include "KeyBinding/InputCommandBinder.h"

CState_Title::CState_Title()
{
	m_titleTexture = NULL;
	memset(m_codeKeysEntered, 0, sizeof(m_codeKeysEntered));
	m_codePos = 0;
	m_demoId = 0;
}

CState_Title::~CState_Title()
{

}

void CState_Title::OnEnter( CBaseStateHandler* from )
{
	g_sounds->Init(100.0f);

	g_sounds->PrecacheSound( "menu.click" );
	g_sounds->PrecacheSound( "menu.thunder" );

	memset(m_codeKeysEntered, 0, sizeof(m_codeKeysEntered));
	m_codePos = 0;
	
	m_demoList.clear();

	// load or reload demo list
	KeyValues kvs;
	if(kvs.LoadFromFile("scripts/rolling_demo.txt"))
	{
		for(int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
		{
			kvkeybase_t* sect = kvs.GetRootSection()->keys[i];

			if(!stricmp(sect->GetName(), "addReplay"))
			{
				const char* demoPath = KV_GetValueString(sect);
				m_demoList.append(demoPath);
			}
		}

		// TODO: use randomUserReplays
	}

	if(m_demoId >= m_demoList.numElem())
		m_demoId = 0;

	m_actionTimeout = 10.0f;
	m_fade = 0.0f;
	m_goesFromTitle = false;
	m_titleTexture = g_pShaderAPI->LoadTexture("ui/title", TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP);
	m_titleTexture->Ref_Grab();

	soundsystem->SetPauseState(false);
	soundsystem->SetVolumeScale(1.0f);
	sndEffect_t* reverb = soundsystem->FindEffect( "menu_reverb" );

	soundsystem->SetListener(vec3_zero, vec3_forward, vec3_up, vec3_zero, reverb);
}

void CState_Title::OnLeave( CBaseStateHandler* to )
{
	g_sounds->Shutdown();

	g_pShaderAPI->FreeTexture(m_titleTexture);
	m_titleTexture = NULL;
}

bool CState_Title::Update( float fDt )
{
	if(!m_goesFromTitle && m_demoList.numElem() > 0)
		m_actionTimeout -= fDt;

	if( m_goesFromTitle || m_actionTimeout <= 0.0f )
	{
		m_fade -= fDt*0.7f;

		if(m_actionTimeout <= 0.0f && m_fade <= 0.0f)
		{
			bool result = g_State_Game->StartReplay( m_demoList[m_demoId++].c_str(), true);

			if(!result)
			{
				m_actionTimeout = 10.0f;
				m_fade = 0.0f;
				m_goesFromTitle = false;
			}

			if(m_demoId >= m_demoList.numElem())
				m_demoId = 0;

			if(result)
				return true;
		}
	}
	else
	{
		m_fade += fDt;
	}

	if(m_titleTexture == NULL)
		return false;

	m_fade = clamp(m_fade, 0.0f, 1.0f);
	m_actionTimeout = max(0.0f, m_actionTimeout);

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	materials->Setup2D(screenSize.x, screenSize.y);
	g_pShaderAPI->Clear( true,true, false, ColorRGBA(0.25f,0,0,0.0f));

	IVector2D halfScreen(screenSize.x/2, screenSize.y/2);

	float widthFac = (float)screenSize.x/(float)m_titleTexture->GetWidth();
	float heightFac = (float)screenSize.y/(float)m_titleTexture->GetHeight();
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

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP,tmprect,elementsOf(tmprect), m_titleTexture, ColorRGBA(fFade,fFade,fFade,1.0f), &blending);

	IEqFont* font = g_fontCache->GetFont("Roboto", 30, TEXT_STYLE_ITALIC);
	IEqFont* copyrightFont = g_fontCache->GetFont("Roboto", 10);

	eqFontStyleParam_t fontParam;
	fontParam.align = TEXT_ALIGN_HCENTER;
	fontParam.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_USE_TAGS;
	fontParam.textColor = color4_white;
	fontParam.scale = 10.0f;

	float textPos = pow(1.0f-m_fade, 4.0f)*(float)screenSize.x*-1.0f;

	float textYOffs = 0.0;

	fontParam.textColor.w = pow(m_fade, 2.0f);

	copyrightFont->RenderText(g_localizer->GetTokenString("INSCOPYRIGHT", L"Copyright (C) Inspiration Byte 2016"), halfScreen + IVector2D(0,300), fontParam);

	if( m_goesFromTitle )
	{
		float sqAlphaValue = clamp(sinf(m_textEffect*32.0f)*16,0.0f,1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

		if(m_textEffect > 0.75f)
			fontParam.textColor.w = sqAlphaValue;
		else
			fontParam.textColor.w = pow(m_textEffect-0.25f, 2.5f);

		textPos = 0.0f;
		textYOffs = (1.0f-m_fade)*7.0f;

		textYOffs = pow(textYOffs, 5.0f);
	}
	else if(m_actionTimeout <= 0.0f)
	{
		textPos = 0.0f;
	}

	m_textEffect -= fDt;

	if(m_textEffect < 0)
		m_textEffect = 0.0f;

	const wchar_t* str = LocalizedString("#MENU_TITLE_PRESS_ENTER");

	Vector2D helloWorldPos(halfScreen.x + textPos, halfScreen.y+150 - textYOffs);

	fontParam.scale = 30.0f;
	font->RenderText(str, helloWorldPos, fontParam);

	return !(m_goesFromTitle && m_fade == 0.0f);
}

int g_KonamiCode[] = {
	KEY_UP,
	KEY_UP,
	KEY_DOWN,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_B,
	KEY_A,
	0,
};

bool CompareCheatCode(int* c1, int* codeSeq)
{
    for ( ; *c1 == *codeSeq; c1++, codeSeq++)
	{
		if (*codeSeq == 0)
			return true;
	}

	return false;
}

void CState_Title::HandleKeyPress( int key, bool down )
{
	if(key == KEY_ENTER && down == true && !m_goesFromTitle && m_fade == 1.0f && m_actionTimeout > 0.0f)
	{
		if(CompareCheatCode(m_codeKeysEntered, g_KonamiCode))
		{
			EmitSound_t es("menu.thunder");
			g_sounds->EmitSound( &es );

			memset(m_codeKeysEntered, 0, sizeof(m_codeKeysEntered));
			m_codePos = 0;

			GetLuaState().call("kkkode");

			return;
		}

		m_goesFromTitle = true;
		m_textEffect = 1.0f;

		EmitSound_t es("menu.click");
		g_sounds->EmitSound( &es );

		SetNextState(g_State_MainMenu);
	}

	if(down)
	{
		m_codeKeysEntered[m_codePos++] = key;

		if(m_codePos >= 16)
		{
			memset(m_codeKeysEntered, 0, sizeof(m_codeKeysEntered));
			m_codePos = 0;
		}
	}

	g_inputCommandBinder->OnKeyEvent( key, down );
}

CState_Title* g_State_Title = new CState_Title();
