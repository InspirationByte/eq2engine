//***********Copyright (C) Illusion Way Interactive Software 2008-2010************
//
// Description: Camera for Engine
//
//****************************************************************************

#include "BaseEngineHeader.h"

#include "ViewParams.h"
#include "GameInput.h"

static ConVar camera_speed("camera_speed","15","Editor camera speed",CV_ARCHIVE);
static ConVar camera_accel("camera_accel","18","Editor camera speed",CV_ARCHIVE);

DECLARE_CMD(create_camera,Spawns new entity,0)
{
	IEntity *pEntity = entityfactory->CreateEntityFromName("editorcamera");
	if(pEntity != NULL)
	{
		pEntity->Spawn();
		pEntity->Activate();
		pEntity->SetAbsOrigin(Vector3D(0));
	}

	/*
	//ScreenToDirection
	Vector2D cursor = eng->GetCursorPosition();

	Vector3D start,dir;
	ScreenToDirection(Vector2D(eng->GetWindowSize().x - cursor.x,cursor.y),start,dir);

	trace_t trace;
	physics->TraceLine(scenerenderer->GetViewParameters()->GetViewOrigin(),dir,2000,COLLISION_GROUP_WORLD,&trace);

	Msg("Creation dir: %f %f %f\n",dir.x,dir.y,dir.z);

	if(trace.fraction != 1.0f)
	{
		IEntity *pEntity = entityfactory->CreateEntityFromName("test_torus");
		if(pEntity != NULL)
		{
			pEntity->Spawn();
			pEntity->Activate();
			pEntity->SetOrigin(trace.traceEnd + (trace.normal * 10));
		}
	}
*/
}

bool recordingMode = false;
bool playingMode = false;

struct demoNode_t
{
	Vector3D position;
	Vector3D angles;
};

DkList<demoNode_t> demoNodes;

char demoName[128];

DECLARE_CMD(demo_record,Record demo,0)
{
	if(!args)
		return;
	if(args->numElem() <= 0)
		return;

	if(recordingMode)
	{
		MsgInfo("Already recording %s.camerademo\n",demoName);
		return;
	}

	demoNodes.clear();

	recordingMode = true;

	MsgInfo("Recording %s.camerademo\n",args->ptr()[0].getData());

	strcpy(demoName,args->ptr()[0].getData());
}

DECLARE_CMD(demo_play,play camera demo,0)
{
	if(!args)
		return;
	if(args->numElem() <= 0)
		return;

	demoNodes.clear();

	strcpy(demoName,args->ptr()[0].getData());

	FILE* file = GetFileSystem()->Open((DkStr(demoName) + ".camerademo").getData(),"rb");
	if(!file)
	{
		MsgWarning("not found %s.camerademo\n",demoName);
		return;
	}

	int count = 0;
	char mapname[64];
	GetFileSystem()->Read(&mapname,sizeof(mapname),1,file);

	if(stricmp(mapname,gpGlobals->mapname))
	{
		Msg("Demo is not for current map\n");
		GetFileSystem()->Close(file);
		return;
	}

	GetFileSystem()->Read(&count,sizeof(int),1,file);

	for(int i = 0; i < count; i++)
	{
		demoNode_t node;
		GetFileSystem()->Read(&node,sizeof(demoNode_t),1,file);
		demoNodes.append(node);
	}

	Msg("read %d frames of demo\n",count);

	GetFileSystem()->Close(file);

	playingMode = true;
}

DECLARE_CMD(demo_end,Stops demo recording,0)
{
	if(recordingMode)
	{
		int count = demoNodes.numElem();
		MsgInfo("Demo completed, %d frames\n",count);

		FILE* file = fopen((DkStr(demoName) + ".camerademo").getData(),"wb");
		if(file)
		{
			fwrite(&gpGlobals->mapname,sizeof(gpGlobals->mapname),1,file);

			fwrite(&count,sizeof(int),1,file);

			fwrite(demoNodes.ptr(),sizeof(demoNode_t) * demoNodes.numElem(),1,file);

			fclose(file);
		}
		demoNodes.clear();

		recordingMode = false;
	}
	else
	{
		MsgWarning("Not recording a demo\n");
	}
}

class CEditorCamera : public BaseEntity
{
public:
	CEditorCamera();
	~CEditorCamera();
	
	void Spawn();
	void Activate();

	void OnGameStart();
	void OnPreRender();
	void OnPostRender();

	void OnRemove();

	void SetAbsOrigin(Vector3D &pos);
	void SetAbsAngles(Vector3D &ang);
	void SetAbsVelocity(Vector3D &vel);

	void Update(float decaytime);

private:
	CViewParams*		m_pCameraParams;
	IEngineModel*		m_pActModel;

	ISoundEmitter*		m_pFootStepEmitter;
	ISoundEmitter*		m_pShootEmitter;
	ISoundSample*		m_pShootSample;
	ISoundSample**		m_pFootStepSamples;

	Vector3D			m_weaponLag;
	Vector3D			m_weaponLastAngle;
	float				nextSetLastAngle;

	float				nextFootStep;
	float				testTime;

	float				shootTime;
	float				lightDieTime;
	Vector3D			m_vDevation;
	Vector3D			m_vLastLightPos;

	Vector3D			m_velocity_lerp;

	Vector3D			m_anglebobVector;
	Vector3D			m_anglebobTimes;

	bonecontroller_t*	m_pitchControl;

	IPhysicsObject*		m_pPhysics;

	bool				m_bFootSound;
};

LINK_ENTITY_TO_CLASS(editorcamera,CEditorCamera)

CEditorCamera::CEditorCamera()
{
	m_pCameraParams		= NULL;
	m_pFootStepEmitter	= NULL;
	m_pFootStepSamples	= NULL;
	m_pShootEmitter		= NULL;
	m_pShootSample		= NULL;
	nextSetLastAngle	= 0;
	nextFootStep		= 0;
	testTime			= 0;
	m_anglebobVector	= Vector3D(0);
	m_anglebobTimes		= Vector3D(0);
	m_vDevation			= Vector3D(0);
	m_vLastLightPos		= Vector3D(0);
	shootTime			= 0;
	lightDieTime		= 0;
	m_pActModel			= NULL;
	m_pitchControl		= NULL;
	m_pPhysics			= NULL;
	//m_bWaitForLanding	= false;
	m_bFootSound		= false;
}

CEditorCamera::~CEditorCamera()
{


}

void CEditorCamera::Activate()
{
	BaseEntity::Activate();

	SetAbsVelocity(Vector3D(0));

	if(m_pCameraParams == NULL)
		m_pCameraParams = new CViewParams();

	if(m_pActModel == NULL)
	{
		m_pActModel = engine->LoadModel("models/actors/default.egf");
		int bone = m_pActModel->FindBone("b_spine_1");
		m_pitchControl = m_pActModel->CreateBoneController(bone);

		if(!m_pitchControl)
		{
			MsgError("no m_pitchControl\n");
		}
	}

	physmodelcreateinfo_t physInfo;
	physInfo.mass = 86.0f;
	physInfo.genConvex = false;
	physInfo.data = NULL;

	SetDefaultPhysModelInfoParams(&physInfo);

	pritimiveinfo_t primInfo;
	primInfo.primType = PHYSPRIM_CAPSULE;
	primInfo.capsuleInfo.height = 52;
	primInfo.capsuleInfo.radius = 28;

	if(!m_pPhysics)
		m_pPhysics = physics->CreatePhysicsObject(&physInfo, &primInfo, COLLISION_GROUP_ACTORS);

	m_pPhysics->SetCollisionMask(COLLIDE_ACTOR);

	m_pPhysics->SetPosition(GetAbsOrigin());

	//scenerenderer->SetView(m_pCameraParams);
}

void CEditorCamera::OnGameStart()
{
	//m_pPhysics->SetFreezeState(false);
}

void CEditorCamera::OnRemove()
{
	if(m_pCameraParams != NULL)
		delete m_pCameraParams;

	m_pActModel->DestroyModel();
	DDelete(m_pActModel);

	if(m_pFootStepSamples)
	{
		for(int i = 0; i < 4; i++)
		{
			if(m_pFootStepSamples[i])
				soundsystem->ReleaseSample(m_pFootStepSamples[i]);
		}
		DDeleteArray(m_pFootStepSamples);
	}

	m_pFootStepSamples = NULL;

	if(m_pFootStepEmitter)
		soundsystem->FreeSoundEmitter(m_pFootStepEmitter);

	m_pFootStepEmitter = NULL;
}

void CEditorCamera::Spawn()
{
	m_pFootStepSamples = DNewArray(ISoundSample*,4);
	for(int i = 0; i < 4; i++)
	{
		m_pFootStepSamples[i] = soundsystem->LoadSoundSample((DkStr("physics/footsteps/conc")+DkStr(varargs("%d.wav",i+1))).getData(),false);
	}

	m_pShootSample = soundsystem->LoadSoundSample("weapons/de/deagle_shoot.wav",false);

	m_pFootStepEmitter = soundsystem->AllocSoundEmitter();
	m_pShootEmitter = soundsystem->AllocSoundEmitter();

	BaseEntity::Spawn();
}

void CEditorCamera::OnPreRender()
{
	Renderable* pRenderable2 = new Renderable;
	pRenderable2->SetModel(m_pActModel);
	pRenderable2->SetOrigin(GetAbsOrigin() + Vector3D(0,-90,0));
	pRenderable2->SetAngles(Vector3D(0, GetAbsAngles().y, 0));
	pRenderable2->SetScale(Vector3D(1));
	//pRenderable2->SetFlags(SO_ALWAYS_VISIBLE);
	pRenderable2->SetBBoxMins(m_pActModel->GetBBOXMins());
	pRenderable2->SetBBoxMaxs(m_pActModel->GetBBOXMaxs());

	scenerenderer->AddToRenderList(pRenderable2);

	m_anglebobTimes.x += gpGlobals->frametime * 14;
	float val = sin(m_anglebobTimes.x)* length(GetAbsVelocity()) * -0.002f;
	m_anglebobVector.x = val;

	m_anglebobTimes.y += gpGlobals->frametime * 7;
	m_anglebobVector.y = sin(m_anglebobTimes.y)* length(GetAbsVelocity()) * 0.004f;

	m_anglebobTimes.z += gpGlobals->frametime * 7;
	m_anglebobVector.z = sin(m_anglebobTimes.z)* length(GetAbsVelocity()) * 0.002f;

	if(nextSetLastAngle < gpGlobals->curtime)
	{
		m_weaponLastAngle = GetAbsAngles();
		nextSetLastAngle = gpGlobals->curtime + 0.01f;
	}
}

void CEditorCamera::OnPostRender()
{
	Vector3D forward, right, up;
	AngleVectors(GetAbsAngles(),&forward,&right,&up);

	m_weaponLag = lerp(m_weaponLag,GetAbsAngles() - m_weaponLastAngle,gpGlobals->frametime*8);
	m_weaponLag = clamp(m_weaponLag,-5,5);

	internaltrace_t ray;
	//physics->InternalTraceBox(m_pCameraParams->GetOrigin() + forward * 15,forward, 2000, Vector3D(10,10,10), 0, &ray);//InternalTraceLine(GetAbsOrigin(),forward, 2000, 0, &ray);

	debugoverlay->Box(ray.traceEnd - Vector3D(10,10,10), ray.traceEnd + Vector3D(10,10,10), Vector4D(0,1,0,1));
	debugoverlay->Line(ray.traceEnd,ray.traceEnd + ray.normal*10,Vector4D(0,0,1,1));

	m_pitchControl->addAngles.x = GetAbsAngles().x;

	static ITexture* pSpot = g_pShaderAPI->LoadTexture("effects/flashlight",TEXFILTER_TRILINEAR_ANISO,ADDRESSMODE_CLAMP,0);
/*
	dlight_t flashlight;
	SetLightDefaults(&flashlight);

	flashlight.position = m_pCameraParams->GetOrigin()+forward*8+right*6+up*8;
	//flashlight.fov = 75;
	//flashlight.angles = GetAbsAngles();
	flashlight.color = ColorRGB(1,0,0);
	flashlight.radius = 600;
	//flashlight.mask = pSpot;

	LS_AddLight(flashlight);
*/
}

void CEditorCamera::SetAbsOrigin(Vector3D &pos)
{
	m_vecOrigin = pos;
}
void CEditorCamera::SetAbsAngles(Vector3D &ang)
{
	m_vecAngles = ang;
}
void CEditorCamera::SetAbsVelocity(Vector3D &vel)
{
	m_vecVelocity = vel;
}

void CEditorCamera::Update(float decaytime)
{
	internaltrace_t groundRay;
//	physics->InternalTraceBox(m_pPhysics->GetPosition()-Vector3D(0,35,0),Vector3D(0,-1,0), 20, Vector3D(10,10,10), 0, &groundRay);

	bool onGround = true;

	if(groundRay.fraction == 1.0)
	{
		onGround = false;
		m_bFootSound = true;
	}

	if(playingMode)
	{
		static Vector3D nextPos;
		static Vector3D lerpPos(0);
		static Vector3D lerpAngle(0);

		static Vector3D interpolatedPos;
		static Vector3D interpolatedVel;
		static Vector3D interpolatedAng;

		static int currentFrame = 0;

		if(currentFrame == 0)
		{
			interpolatedPos = demoNodes[currentFrame].position;
			interpolatedAng = demoNodes[currentFrame].angles;

			currentFrame++;

			nextPos = demoNodes[currentFrame].position;

			interpolatedVel = nextPos - interpolatedPos;
			lerpPos = interpolatedPos;
		}

		if(currentFrame >= demoNodes.numElem()-1)
		{
			playingMode = false;
			currentFrame = 0;
			demoNodes.clear();
			return;
		}

		interpolatedPos += interpolatedVel * 0.5f * gpGlobals->frametime;

		lerpAngle = lerp(lerpAngle, interpolatedAng,gpGlobals->frametime * 5);
		lerpPos = lerp(lerpPos, interpolatedPos, gpGlobals->frametime * 5);

		// clamped distance
		float distance = length(nextPos - interpolatedPos) * 0.0055f;

		interpolatedAng = lerp(interpolatedAng,lerp(demoNodes[currentFrame].angles,demoNodes[currentFrame-1].angles,distance - distance*0.01f),gpGlobals->frametime * 1);
		debugoverlay->Text(Vector4D(1)," DISTANCE: %f", distance);

		if(distance < 0.01f)
		{
			currentFrame++;
			nextPos = demoNodes[currentFrame].position;
			interpolatedVel = nextPos - interpolatedPos;
		}

		m_pCameraParams->SetOrigin(lerpPos);
		m_pCameraParams->SetAngles(lerpAngle);

		return;
	}

	Vector3D cam_forward;
	Vector3D cam_right;
	Vector3D cam_up;
	AngleVectors(GetAbsAngles(),&cam_forward,&cam_right,&cam_up);

	static int headBone = m_pActModel->FindBone("b_head");

	Vector3D bone_orig = m_pActModel->GetBonePosition(headBone);
	VectorRotate(m_pActModel->GetBonePosition(headBone),-Vector3D(0, GetAbsAngles().y,0),&bone_orig);

	m_pCameraParams->SetOrigin(GetAbsOrigin() + bone_orig + Vector3D(0,-90,0));
	m_pCameraParams->SetAngles(GetAbsAngles() + m_vDevation);

	m_pCameraParams->SetFOV(100);

	Vector3D forward;
	Vector3D right;

	AngleVectors(Vector3D(0, GetAbsAngles().y, 0),&forward,&right,NULL);

	bool bAnyButtonPressed = false;

	Vector3D	velocity(0);

	m_pActModel->AdvanceFrame(gpGlobals->frametime);

	static float nextAnimTime = 0;

	if(bAnyButtonPressed && onGround)
		m_pActModel->SetAnimation(1);
	else
		m_pActModel->SetAnimation(0);

	if(bAnyButtonPressed)
		m_pPhysics->SetFreezeState(false);

	if(nextAnimTime < gpGlobals->curtime)
	{
		m_pActModel->ResetTimers();
		nextAnimTime = gpGlobals->curtime + m_pActModel->GetCurrAnimDuration();
	}

	//scenerenderer->SetView(m_pCameraParams);

	m_vDevation = lerp(m_vDevation,Vector3D(0),gpGlobals->frametime * 2);

	float spd = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;
	if((spd > 0) && ( spd > camera_speed.GetFloat()*camera_speed.GetFloat() ))
	{
		float fRatio = camera_speed.GetFloat() / sqrtf( spd );
		velocity *= fRatio;
	}

	m_pFootStepEmitter->SetPosition(GetAbsOrigin() - Vector3D(0,55,0));

	if(nextFootStep < gpGlobals->curtime && length(m_pPhysics->GetVelocity().xz()) > 40 && onGround || (onGround && m_bFootSound && nextFootStep < gpGlobals->curtime))
	{
		m_bFootSound = false;
		m_pFootStepEmitter->Play(m_pFootStepSamples[RandomInt(0,3)]);
		nextFootStep = gpGlobals->curtime + 0.37f * RandomFloat(0.9f,1.1f);
	}

	if(onGround)
		m_velocity_lerp = lerp(m_velocity_lerp,velocity * camera_accel.GetFloat(),gpGlobals->frametime * 8);

	m_velocity_lerp = clamp(m_velocity_lerp,-camera_speed.GetFloat(),camera_speed.GetFloat());

	m_velocity_lerp.y = 0;

	if(onGround)
		m_pPhysics->SetVelocity(m_velocity_lerp * camera_speed.GetFloat() + Vector3D(0,m_pPhysics->GetVelocity().y,0));

	// Get frame time from engine because we doesn't playing in edit mode
	//m_vecOrigin += GetAbsVelocity() * gpGlobals->frametime;
	m_vecOrigin = m_pPhysics->GetPosition() + Vector3D(0,48,0);
	m_pPhysics->SetAngles(Vector3D(0));
}