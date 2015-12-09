\
//-=-=-=-=-=-=-=-=-=-= Copyright (C) Damage Byte L.L.C 2009-2012 =-=-=-=-=-=-=-=-=-=-=-
//
// Description: Demo camera
//
//-=-=-=-D-=-A-=-M-=-A-=-G-=-E-=-=-B-=-Y-=-T-=-E-=-=-S-=-T-=-U-=-D-=-I-=-O-=-S-=-=-=-=-

#include "BaseEngineHeader.h"
#include "BaseActor.h"
#include "GameInput.h"
#include "IEngineHost.h"
#include "GameRenderer.h"

struct democameranode_t
{
	Vector3D	position;
	Vector3D	angles;

	float		fov;
	//float		fade;
};

DkList<democameranode_t> g_demonodesList;

#define DEMO_NODEMO		0
#define DEMO_RECORD		1
#define DEMO_PLAY		2

int g_nDemoRecordStatus = DEMO_NODEMO;
EqString g_szDemoName("Invalid");

int g_nDemoFrameA = 0;
int g_nDemoFrameB = 0;

float g_fDemoTime = 0;

void ResetDemoTime()
{
	g_nDemoFrameA = 0;
	g_nDemoFrameB = 0;
	g_fDemoTime = 0;
}

void SaveDemo(const char* filename)
{
	DKFILE* file = GetFileSystem()->Open(filename, "wb", SP_MOD);
	if(!file)
	{
		Msg("Can't write demo to file %s\n", filename);
		return;
	}

	// write map name
	GetFileSystem()->Write(gpGlobals->worldname, sizeof(gpGlobals->worldname), 1, file);

	int numFrames = g_demonodesList.numElem();

	// write frame count
	GetFileSystem()->Write(&numFrames, sizeof(int), 1, file);

	// write all nodes
	for(int i = 0; i < numFrames; i++)
		GetFileSystem()->Write(&g_demonodesList[i], sizeof(democameranode_t), 1, file);

	// close file
	GetFileSystem()->Close(file);
}

void LoadDemo(const char* filename)
{
	g_demonodesList.clear();

	DKFILE* file = GetFileSystem()->Open(filename, "rb", SP_MOD);

	if(!file)
	{
		Msg("Can't read demo from file %s\n", filename);
		return;
	}

	char mapname[64];
	// read map name
	GetFileSystem()->Read(gpGlobals->worldname, sizeof(gpGlobals->worldname),1,file);

	if(stricmp(mapname, gpGlobals->worldname))
	{
		GetFileSystem()->Close(file);
		Msg("Demo is not for this map\n");
		return;
	}

	int numFrames = 0;

	// read frame count
	GetFileSystem()->Read(&numFrames, sizeof(int), 1, file);

	g_demonodesList.setNum(numFrames);

	// read all nodes
	for(int i = 0; i < numFrames; i++)
		GetFileSystem()->Read(&g_demonodesList[i], sizeof(democameranode_t), 1, file);

	// close file
	GetFileSystem()->Close(file);
}

void AddDemoNode(Vector3D &origin, Vector3D &angles, float fFOV)
{
	democameranode_t node;
	node.position	= origin;
	node.angles		= angles;
	node.fov		= fFOV;

	g_demonodesList.append(node);
}
/*
int g_nDemoFrameA = 0;
int g_nDemoFrameB = 0;
float g_fDemoTime = 0;
*/

// advances frame (and computes interpolation between all frames)
void AdvanceDemoFrame(float frameTime)
{
	if(g_nDemoRecordStatus != DEMO_PLAY)
		return;

	// increment frames
	g_fDemoTime += frameTime;

	// compute frame numbers
	g_nDemoFrameA = floor(g_fDemoTime+1)-1;
	g_nDemoFrameB = g_nDemoFrameA+1;

	int curNumFrames = g_demonodesList.numElem();

	// check frame bounds
	if(g_nDemoFrameB >= curNumFrames)
	{
		// set back
		g_nDemoFrameB = curNumFrames-1;
	}

	if(g_nDemoFrameA > curNumFrames-2)
	{
		g_nDemoRecordStatus = DEMO_NODEMO;
		g_nDemoFrameA = curNumFrames-2;
	}

	// set end time
	g_fDemoTime = clamp(g_fDemoTime, 0, curNumFrames-1);

	if(g_nDemoFrameA < 0)
		g_nDemoFrameA = 0;

	if(g_nDemoFrameB < 0)
		g_nDemoFrameB = 0;
}

DECLARE_CMD(demo_stop,"Stops demo recording or playing", 0)
{
	if(g_nDemoRecordStatus == DEMO_NODEMO)
	{
		MsgWarning("Do demo recording or playing\n");
		return;
	}

	if(g_nDemoRecordStatus == DEMO_RECORD)
	{
		// save demo
		SaveDemo(g_szDemoName.GetData());

		ResetDemoTime();

		Msg("Demo saved to %s\n", g_szDemoName.GetData());

		g_demonodesList.clear();

		g_nDemoRecordStatus = DEMO_NODEMO;
	}

	if(g_nDemoRecordStatus == DEMO_PLAY)
	{
		// stop demo
		g_nDemoRecordStatus = DEMO_NODEMO;

		ResetDemoTime();

		g_demonodesList.clear();
	}
}

int oldButtons = 0;

class CDemoCamera : public BaseEntity
{
	DEFINE_CLASS_BASE(BaseEntity);

public:
	CDemoCamera()
	{
		m_view.SetFOV(90);
		m_view.SetOrigin(vec3_zero);
		m_view.SetAngles(vec3_zero);

		m_szDemoName = "";

		original_angles_ptr = NULL;
		m_original_view = NULL;
	}

	void OnRemove()
	{
		if(original_angles_ptr)
			pEyeAngles = original_angles_ptr;

		if(m_original_view)
			viewrenderer->SetView(m_original_view);

		g_nDemoRecordStatus = DEMO_NODEMO;

		ResetDemoTime();

		g_demonodesList.clear();

		BaseClass::OnRemove();
	}

	void Play()
	{
		ResetDemoTime();
		g_szDemoName = m_szDemoName + _Es(".cdm");
		LoadDemo(g_szDemoName.GetData());

		if(g_demonodesList.numElem() == 0)
		{
			SetState(ENTITY_REMOVE);
			return;
		}

		g_nDemoRecordStatus = DEMO_PLAY;

		SetupCamera();
	}

	void SetupCamera()
	{
		BaseEntity* pPlayer = UTIL_EntByIndex(1);
		CBaseActor* pActor = dynamic_cast<CBaseActor*>(pPlayer);

		if(pActor)
		{
			//SetAbsOrigin(pActor->GetEyeOrigin());
			camera_angles = pActor->GetEyeAngles();
		}

		original_angles_ptr = pEyeAngles;

		m_original_view = viewrenderer->GetView();

		if(g_nDemoRecordStatus == DEMO_RECORD)
			pEyeAngles = &camera_angles;

		viewrenderer->SetView(&m_view);

		if(m_original_view)
			m_view.SetFOV(m_original_view->GetFOV());
	}

	void SetDemoPath(const char *pszName)
	{
		m_szDemoName = pszName;
	}

	void OnGameStart()
	{
		if(m_szDemoName.GetLength() > 0)
			Play();
	}

	void OnPreRender()
	{
		if(g_nDemoRecordStatus == DEMO_RECORD)
		{
			m_view.SetOrigin(GetAbsOrigin());
			m_view.SetAngles(camera_angles);

			if(nClientButtons & IN_JUMP && !(oldButtons & IN_JUMP))
			{
				AddDemoNode(GetAbsOrigin(), camera_angles, m_view.GetFOV());
				Msg("Frame added\n");
			}

			if(nClientButtons & IN_SPD_FASTER)
			{
				camera_angles.z += 1;

				camera_angles.z = clamp(camera_angles.z,-180,180);

				UNSETFLAG(nClientButtons,IN_SPD_FASTER);
			}

			if(nClientButtons & IN_SPD_SLOWER)
			{
				camera_angles.z -= 1;

				camera_angles.z = clamp(camera_angles.z,-180,180);

				UNSETFLAG(nClientButtons,IN_SPD_SLOWER);
			}

			if(nClientButtons & IN_PRIMARY)
			{
				m_view.SetFOV(m_view.GetFOV() - 1.0f);

				if(m_view.GetFOV() < 10)
					m_view.SetFOV(10);

				if(m_view.GetFOV() > 120)
					m_view.SetFOV(120);
			}

			if(nClientButtons & IN_SECONDARY)
			{
				m_view.SetFOV(m_view.GetFOV() + 1.0f);

				if(m_view.GetFOV() < 10)
					m_view.SetFOV(10);

				if(m_view.GetFOV() > 120)
					m_view.SetFOV(120);
			}

			Vector3D moveDir(0);
			Vector3D fw,rt;

			AngleVectors(camera_angles, &fw, &rt);

			if(nClientButtons & IN_FORWARD)
			{
				moveDir += fw;
			}
			if(nClientButtons & IN_BACKWARD)
			{
				moveDir -= fw;
			}
			if(nClientButtons & IN_STRAFELEFT)
			{
				moveDir -= rt;
			}
			if(nClientButtons & IN_STRAFERIGHT)
			{
				moveDir += rt;
			}

			if(nClientButtons & IN_FORCERUN)
				moveDir *= 350;
			else
				moveDir *= 150;

			SetAbsOrigin(GetAbsOrigin() + moveDir*g_pEngineHost->GetFrameTime());
		}
		else if(g_nDemoRecordStatus == DEMO_PLAY)
		{
			// setup basic position
			if(g_fDemoTime == 0.0f)
			{
				camera_interp_pos = g_demonodesList[0].position;
				camera_interp_ang = g_demonodesList[0].angles;
				camera_interp_fov = g_demonodesList[0].fov;
			}

			AdvanceDemoFrame(gpGlobals->frametime);

			float frameInterp = g_fDemoTime - g_nDemoFrameA;

			Vector3D iterpPos = lerp(g_demonodesList[g_nDemoFrameA].position, g_demonodesList[g_nDemoFrameB].position, frameInterp);
			Vector3D iterpAngles = lerp(g_demonodesList[g_nDemoFrameA].angles, g_demonodesList[g_nDemoFrameB].angles, frameInterp);
			float iterpFOV = lerp(g_demonodesList[g_nDemoFrameA].fov, g_demonodesList[g_nDemoFrameB].fov, frameInterp);

			camera_interp_pos = lerp(camera_interp_pos, iterpPos, gpGlobals->frametime*2);
			camera_interp_ang = lerp(camera_interp_ang, iterpAngles, gpGlobals->frametime*2);
			camera_interp_fov = lerp(camera_interp_fov, iterpFOV, gpGlobals->frametime*2);

			m_view.SetOrigin(camera_interp_pos);
			m_view.SetAngles(camera_interp_ang);
			m_view.SetFOV(camera_interp_fov);

			if(nClientButtons == IN_JUMP)
				g_nDemoRecordStatus = DEMO_NODEMO;
		}
		else if(g_nDemoRecordStatus == DEMO_NODEMO)
		{
			// remove this entity
			SetState(ENTITY_REMOVE);
		}

		oldButtons = nClientButtons;
	}

	void OnPostRender()
	{
		if(g_nDemoRecordStatus == DEMO_RECORD)
		{
			g_pEngineHost->GetDefaultFont()->DrawSetColor(ColorRGBA(1,1,0,1));
			g_pEngineHost->GetDefaultFont()->DrawText(varargs("Camera demo frames recorded: %d", g_demonodesList.numElem()),g_pEngineHost->GetWindowSize().x/2,g_pEngineHost->GetWindowSize().y/2, 16,25,false);
			g_pEngineHost->GetDefaultFont()->DrawText("Press 'JUMP' button to record frame",g_pEngineHost->GetWindowSize().x/2,g_pEngineHost->GetWindowSize().y/2+28, 16,25,false);
		}

		//UTIL_DrawScreenRect(Vector2D(0,0), Vector2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y / 5), 0, ColorRGBA(0,0,0,1));
		//UTIL_DrawScreenRect(Vector2D(0,g_pEngineHost->GetWindowSize().y - g_pEngineHost->GetWindowSize().y / 5), Vector2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y), 0, ColorRGBA(0,0,0,1));
	}

	DECLARE_DATAMAP()

protected:


	CViewParams		m_view;

	BaseEntity*		m_original_view;

	Vector3D		camera_angles;
	Vector3D*		original_angles_ptr;

	Vector3D		camera_interp_pos;
	Vector3D		camera_interp_ang;
	float			camera_interp_fov;

	EqString		m_szDemoName;
};

BEGIN_DATAMAP(CDemoCamera)

	DEFINE_KEYFIELD( m_szDemoName, "demoname", VTYPE_STRING),

END_DATAMAP()

DECLARE_ENTITYFACTORY(democamera, CDemoCamera)

DECLARE_CMD(demo_play,"Plays a demo", 0)
{
	if(g_nDemoRecordStatus == DEMO_RECORD)
	{
		MsgWarning("Can't play when recording\n");
		return;
	}

	if(g_nDemoRecordStatus == DEMO_PLAY)
	{
		MsgWarning("Can't play again when playing\n");
		return;
	}

	if(args && args->numElem() > 0)
	{
		CDemoCamera* pDemoRectorder = (CDemoCamera*)entityfactory->CreateEntityByName("democamera");
		if(pDemoRectorder)
		{
			g_szDemoName = args->ptr()[0];

			pDemoRectorder->SetDemoPath(g_szDemoName.GetData());
			pDemoRectorder->Play();
			pDemoRectorder->Spawn();
			pDemoRectorder->Activate();
		}
	}
	else
	{
		MsgWarning("You doesn't specified demo name\n");
	}
}

DECLARE_CMD(demo_record,"Records demo", 0)
{
	if(g_nDemoRecordStatus == DEMO_RECORD)
	{
		MsgWarning("Already recording demo\n");
		return;
	}

	if(g_nDemoRecordStatus == DEMO_PLAY)
	{
		MsgWarning("Demo is playing, run demo_stop\n");
		return;
	}

	if(args && args->numElem() > 0)
	{
		ResetDemoTime();

		g_szDemoName = args->ptr()[0] + _Es(".cdm");
		g_nDemoRecordStatus = DEMO_RECORD;
		g_demonodesList.clear();
		MsgInfo("Recording demo '%s'\n", g_szDemoName.GetData());

		CDemoCamera* pDemoRectorder = (CDemoCamera*)entityfactory->CreateEntityByName("democamera");
		if(pDemoRectorder)
		{
			pDemoRectorder->SetupCamera();
			pDemoRectorder->Spawn();
			pDemoRectorder->Activate();
		}
	}
	else
	{
		MsgWarning("You doesn't specified demo name\n");
	}
}