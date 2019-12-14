//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: water flow of fire hydrant
//////////////////////////////////////////////////////////////////////////////////

#include "object_waterflow.h"
#include "world.h"

#include "EqParticles.h"

#include "ParticleEffects.h"

extern CPFXAtlasGroup* g_translParticles;

const float WATERFLOW_RADIUS = 0.5f;
const float WATERFLOW_VELOCITY = 18.0f;
const float WATERFLOW_ANGLE = 2.0f;

const float FORCE_REDUCE_TIME = 5.0f;

CObject_WaterFlow::CObject_WaterFlow( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_nextRippleTime = 0.0f;
	m_waterFlowSound = nullptr;
}

CObject_WaterFlow::~CObject_WaterFlow()
{

}

void CObject_WaterFlow::OnRemove()
{
	BaseClass::OnRemove();

	g_pPhysics->m_physics.DestroyGhostObject(m_ghostObject);
	m_ghostObject = NULL;

	g_sounds->RemoveSoundController(m_waterFlowSound);
}

void CObject_WaterFlow::Spawn()
{
	BaseClass::Spawn();

	m_lifeTime = KV_GetValueFloat(m_keyValues->FindKeyBase("lifetime"), 0, 10.0f);
	m_force = KV_GetValueFloat(m_keyValues->FindKeyBase("force"), 0, 15.0f);

	CollisionData_t collData;
	if (g_pPhysics->TestLine(GetOrigin(), GetOrigin() - vec3_up * 5.0f, collData, OBJECTCONTENTS_SOLID_GROUND))
	{
		m_groundPos = collData.position;
		m_groundNormal = collData.normal;
	}
	else
	{
		m_groundPos = GetOrigin();
		m_groundNormal = vec3_up;
	}

	const char* soundName = KV_GetValueString(m_keyValues->FindKeyBase("sound"), 0, nullptr);

	if (soundName)
	{
		g_sounds->PrecacheSound(soundName);

		EmitSound_t soundEp;
		soundEp.name = soundName;
		soundEp.fPitch = 1.0f;
		soundEp.fVolume = 1.0f;
		soundEp.fRadiusMultiplier = 1.0f;
		soundEp.origin = m_vecOrigin;
		soundEp.pObject = this;

		m_waterFlowSound = g_sounds->CreateSoundController(&soundEp);
		m_waterFlowSound->Play();
	}

	m_ghostObject = new CEqCollisionObject();

	m_ghostObject->Initialize( WATERFLOW_RADIUS );

	m_ghostObject->SetCollideMask(0);
	m_ghostObject->SetContents(OBJECTCONTENTS_DEBRIS);

	m_ghostObject->SetPosition(m_groundPos);

	m_ghostObject->SetUserData(this);

	m_bbox = m_ghostObject->m_aabb_transformed;

	// Next call will add NO_RAYCAST, COLLISION_LIST and DISABLE_RESPONSE flags automatically
	g_pPhysics->m_physics.AddGhostObject( m_ghostObject );
}

void CObject_WaterFlow::Simulate( float fDt )
{
	if (fDt <= 0.0f)
		return;

	m_lifeTime -= fDt;

	if (m_lifeTime < 0.0f)
	{
		m_state = GO_STATE_REMOVE;
		return;
	}

	float intensity = RemapValClamp(m_lifeTime, 0.0f, FORCE_REDUCE_TIME, 0.0f, 1.0f);

	// add fountain
	Vector3D ripplePos = m_groundPos + Vector3D(RandomFloat(-0.25f, 0.25f), 0.05f, RandomFloat(-0.25f, 0.25f));
	MakeWaterSplash(ripplePos - vec3_up*0.5f, vec3_up * WATERFLOW_VELOCITY * intensity, Vector3D(WATERFLOW_ANGLE), 0.25f + intensity*4.0f, 1, 0.25f + intensity*0.5f);

	m_nextRippleTime -= fDt;

	if(m_nextRippleTime < 0.0f)
	{
		CRippleEffect* ripple = new CRippleEffect(ripplePos, 1.5f*intensity, 3.0f*intensity+2.5f, 0.5f, 
			g_translParticles, g_worldGlobals.trans_rainripple, 
			color3_white, m_groundNormal,
			RandomFloat(-15.0f, 15.0f));
		effectrenderer->RegisterEffectForRender(ripple);

		m_nextRippleTime = 0.15f;
	}

	if (m_waterFlowSound)
		m_waterFlowSound->SetVolume(intensity);

	if(m_ghostObject->m_collisionList.numElem() > 0 && m_ghostObject->m_collisionList[0].bodyB)
	{
		CEqCollisionObject* obj = m_ghostObject->m_collisionList[0].bodyB;

		// apply force to the object
		if (obj->IsDynamic())
		{
			CEqRigidBody* body = (CEqRigidBody*)obj;
			body->ApplyWorldImpulse(m_groundPos, Vector3D(vec3_up) * m_force * intensity);
		}
	}

	// we have to deal with cleaning up collision list
	m_ghostObject->m_collisionList.clear( false );
}
