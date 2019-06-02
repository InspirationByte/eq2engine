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

#include "materialsystem/MeshBuilder.h"

CState_Title::CState_Title()
{
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
	g_sounds->PrecacheSound( "menu.titlemusic" );

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

	soundsystem->SetPauseState(false);
	sndEffect_t* reverb = soundsystem->FindEffect( "menu_reverb" );

	soundsystem->SetListener(vec3_zero, vec3_forward, vec3_up, vec3_zero, reverb);

	m_uiLayout = equi::Manager->CreateElement("HudElement");

	kvkeybase_t uiKvs;
	
	if (KV_LoadFromFile("resources/ui_title.res", SP_MOD, &uiKvs))
		m_uiLayout->InitFromKeyValues(&uiKvs);

	m_titleText = m_uiLayout->FindChild("title");

	if (m_titleText)
		m_titlePos = m_titleText->GetPosition();

	EmitSound_t es("menu.titlemusic");
	g_sounds->Emit2DSound(&es);

}

void CState_Title::OnLeave( CBaseStateHandler* to )
{
	g_sounds->Shutdown();

	delete m_uiLayout;
	m_uiLayout = nullptr;
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

	const IVector2D& screenSize = g_pHost->GetWindowSize();

	m_fade = clamp(m_fade, 0.0f, 1.0f);
	m_actionTimeout = max(0.0f, m_actionTimeout);

	ColorRGBA titleTextCol(1.0f);

	if (m_goesFromTitle)
	{
		float sqAlphaValue = clamp(sinf(m_textEffect*32.0f) * 16, 0.0f, 1.0f); //sin(g_pHost->m_fGameCurTime*8.0f);

		if (m_textEffect > 0.75f)
			titleTextCol.w = sqAlphaValue;
		else
			titleTextCol.w = pow(m_textEffect - 0.25f, 2.5f);

		//textPos = 0.0f;
		float textYOffs = (1.0f - m_fade)*7.0f;
		textYOffs = pow(textYOffs, 5.0f);

		if (m_titleText)
			m_titleText->SetPosition(m_titlePos - IVector2D(0, textYOffs));

	}
	else if (m_actionTimeout <= 0.0f)
	{
		//textPos = 0.0f;
	}
	else
	{
		if (m_titleText)
			m_titleText->SetPosition(m_titlePos - IVector2D(pow(1.0f - m_fade, 4.0f) * screenSize.x, 0));
	}

	m_textEffect -= fDt;

	CMeshBuilder meshBuilder(materials->GetDynamicMesh());

	materials->Setup2D(screenSize.x, screenSize.y);

	if(m_titleText)
		m_titleText->SetTextColor(titleTextCol);

	// draw EqUI
	m_uiLayout->SetSize(screenSize);
	m_uiLayout->Render();

	ColorRGBA blockCol(0.0f, 0.0f, 0.0f, 1.0f-m_fade);

	Vector2D screenRect[] = { MAKEQUAD(0, 0,screenSize.x, screenSize.y, 0) };

	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(NULL, NULL, 0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false, false);
	materials->BindMaterial(materials->GetDefaultMaterial());

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		meshBuilder.Color4fv(blockCol);
		meshBuilder.Quad2(screenRect[0], screenRect[1], screenRect[2], screenRect[3]);
	meshBuilder.End();

	// fade music
	if(m_goesFromTitle)
		g_sounds->Set2DChannelsVolume(CHAN_STREAM, m_fade);

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
	if((key == KEY_ENTER || key == KEY_JOY_START) && down == true && !m_goesFromTitle && m_fade == 1.0f && m_actionTimeout > 0.0f)
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
