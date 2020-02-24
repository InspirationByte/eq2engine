//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor test drive
//////////////////////////////////////////////////////////////////////////////////

#include "EditorTestDrive.h"
#include "CameraAnimator.h"
#include "world.h"
#include "input.h"

#include "wx/defs.h"

#include "utils/KeyValues.h"

static CEditorTestGame s_editorTestGame;
CEditorTestGame* g_editorTestGame = &s_editorTestGame;

static CCameraAnimator	s_CameraAnimator;
extern CCameraAnimator*	g_pCameraAnimator = &s_CameraAnimator;;

CEditorTestGame::CEditorTestGame() : m_car(nullptr), m_clientButtons(0)
{

}

CEditorTestGame::~CEditorTestGame()
{

}

void CEditorTestGame::Init()
{
	PrecacheObject( CCar );
}

void CEditorTestGame::Destroy()
{
	EndGame();
}

//-------------------------------------------

bool CEditorTestGame::IsGameRunning() const
{
	return m_car != NULL;
}

bool CEditorTestGame::BeginGame( const char* carName, const Vector3D& startPos )
{
	bool wasGameRunning = IsGameRunning();

	if(wasGameRunning)
	{
		if(m_car)
			m_car->Remove();

		m_car = NULL;
	}

	m_car = CreateCar( carName );

	if(!m_car)
	{
		return false;
	}

	m_car->Precache();

	g_pCameraAnimator->Reset();

	m_car->SetOrigin(startPos);

	if(!wasGameRunning)
	{
		g_pPhysics->SceneInitBroadphase();

		g_pGameWorld->m_level.Ed_InitPhysics();

		// editor create objects
		g_pGameWorld->QueryNearestRegions(m_car->GetOrigin(), true);
	}

	m_car->L_Activate();

	m_car->AlignToGround();

	m_car->m_isLocalCar = true;

	m_clientButtons = 0;

	return true;
}

void CEditorTestGame::EndGame()
{
	// editor create objects
	g_pGameWorld->m_level.Ed_DestroyPhysics();

	if(m_car)
		m_car->Remove();

	m_car = nullptr;

	delete m_carEntry;
	m_carEntry = nullptr;

	g_pPhysics->SceneDestroyBroadphase();
	m_clientButtons = 0;
}

void CEditorTestGame::Update( float fDt )
{
	g_pGameWorld->QueryNearestRegions(m_car->GetOrigin());

	m_car->SetControlButtons( m_clientButtons );
	m_car->GetPhysicsBody()->Wake();

	g_pPhysics->Simulate(fDt, 4, nullptr);

	g_pCameraAnimator->Update( fDt, m_clientButtons, m_car );
}

#define KEYBUTTON_BIT(chr, bit)		\
	if(keyCode == (int)chr)				\
	{								\
		if(down)					\
			m_clientButtons |= bit;	\
		else						\
			m_clientButtons &= ~bit;\
	}


void CEditorTestGame::OnKeyPress(int keyCode, bool down)
{
	KEYBUTTON_BIT('W', IN_ACCELERATE);
	KEYBUTTON_BIT('S', IN_BRAKE);
	KEYBUTTON_BIT('A', IN_STEERLEFT);
	KEYBUTTON_BIT('D', IN_STEERRIGHT);
	KEYBUTTON_BIT(WXK_SPACE, IN_HANDBRAKE);
	KEYBUTTON_BIT(WXK_SHIFT, IN_BURNOUT);
	KEYBUTTON_BIT(WXK_CONTROL, IN_FASTSTEER);
	KEYBUTTON_BIT('H', IN_HORN);
	KEYBUTTON_BIT('C', IN_CHANGECAM);
}

//-------------------------------------------

CCar* CEditorTestGame::CreateCar(const char* name)
{
	EqString configPath = CombinePath(2, "scripts/vehicles", _Es(name) + ".txt");

	kvkeybase_t kvb;
	if (!KV_LoadFromFile(configPath.c_str(), SP_MOD, &kvb))
	{
		MsgError("can't load car script '%s'\n", name);
		return nullptr;
	}

	vehicleConfig_t* conf = new vehicleConfig_t();
	conf->carName = name;
	conf->carScript = name;

	conf->scriptCRC = 0;

	if (!ParseVehicleConfig(conf, &kvb))
		return nullptr;

	m_carEntry = conf;

	return new CCar(m_carEntry);
}