//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor test drive
//////////////////////////////////////////////////////////////////////////////////

#include "EditorTestDrive.h"
#include "CameraAnimator.h"

#include "wx/defs.h"

#include "utils/KeyValues.h"

static CEditorTestGame s_editorTestGame;
CEditorTestGame* g_editorTestGame = &s_editorTestGame;

static CCameraAnimator	s_CameraAnimator;
extern CCameraAnimator*	g_pCameraAnimator = &s_CameraAnimator;;

CEditorTestGame::CEditorTestGame() : m_car(NULL), m_clientButtons(0)
{

}

CEditorTestGame::~CEditorTestGame()
{

}

void CEditorTestGame::Init()
{
	PrecacheObject( CCar );

	// initialize vehicle list
	KeyValues kvs;
	if(kvs.LoadFromFile("scripts/vehicles.txt"))
	{
		kvkeybase_t* vehicleRegistry = kvs.GetRootSection()->FindKeyBase("vehicles");

		for(int i = 0; i < vehicleRegistry->keys.numElem(); i++)
		{
			kvkeybase_t* key = vehicleRegistry->keys[i];
			if(!key->IsSection() && !key->IsDefinition())
			{
				carConfigEntry_t* carent = new carConfigEntry_t();
				carent->carName = key->name;
				carent->carScript = KV_GetValueString(key);

				kvkeybase_t kvs;

				if( !KV_LoadFromFile(carent->carScript.c_str(), SP_MOD,&kvs) )
				{
					MsgError("Can't open car script '%s'\n", carent->carScript.c_str());
					delete carent;
					return;
				}

				if(!ParseCarConfig(carent, &kvs))
				{
					MsgError("Car configuration '%s' is invalid!\n", carent->carScript.c_str());
					delete carent;
					return;
				}

				PrecacheStudioModel( carent->m_cleanModelName.c_str() );
				PrecacheStudioModel( carent->m_damModelName.c_str() );
				m_carEntries.append(carent);
			}
		}
	}
	else
	{
		MsgError("FATAL: no scripts/vehicles.txt file!");
	}
}

void CEditorTestGame::Destroy()
{
	EndGame();

	// delete old entries
	for(int i = 0; i < m_carEntries.numElem(); i++)
		delete m_carEntries[i];

	m_carEntries.clear(false);
}

//-------------------------------------------

bool CEditorTestGame::IsGameRunning() const
{
	return m_car != NULL;
}

void CEditorTestGame::BeginGame( const char* carName, const Vector3D& startPos )
{
	g_pCameraAnimator->Reset();

	g_pPhysics->SceneInitBroadphase();

	m_car = CreateCar( carName );

	m_car->SetOrigin(startPos);
	m_car->L_Activate();

	m_car->AlignToGround();

	m_car->m_isLocalCar = true;

	m_clientButtons = 0;
}

void CEditorTestGame::EndGame()
{
	g_pPhysics->SceneDestroyBroadphase();

	if(m_car)
		m_car->Remove();

	m_car = NULL;

	m_clientButtons = 0;
}

void CEditorTestGame::Update( float fDt )
{
	m_car->SetControlButtons( m_clientButtons );
	m_car->GetPhysicsBody()->Wake();

	g_pPhysics->Simulate(fDt, 4, NULL);

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
	KEYBUTTON_BIT('A', IN_TURNLEFT);
	KEYBUTTON_BIT('D', IN_TURNRIGHT);
	KEYBUTTON_BIT(WXK_SPACE, IN_HANDBRAKE);
	KEYBUTTON_BIT(WXK_SHIFT, IN_BURNOUT);
	KEYBUTTON_BIT(WXK_CONTROL, IN_EXTENDTURN);
	KEYBUTTON_BIT('H', IN_HORN);
	KEYBUTTON_BIT('C', IN_CHANGECAM);
}

//-------------------------------------------

CCar* CEditorTestGame::CreateCar(const char* name) const
{
	for(int i = 0; i < m_carEntries.numElem(); i++)
	{
		if(!m_carEntries[i]->carName.CompareCaseIns(name))
		{
			return new CCar(m_carEntries[i]);
		}
	}

	return NULL;
}