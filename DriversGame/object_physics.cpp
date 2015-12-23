//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: physics object
//////////////////////////////////////////////////////////////////////////////////

#include "object_physics.h"
#include "session_stuff.h"

#include "world.h"

#include "IDebugOverlay.h"

#include "state_game.h"

#include "Shiny.h"

void EmitHitSoundEffect(CGameObject* obj, const char* prefix, const Vector3D& origin, float power, float maxPower)
{
	float fSoundPowerPercent = power / maxPower;

	if (fSoundPowerPercent < 0.1)
		return;

	//Msg("hit sound '%s', power: %g, max power: %g, perc: %g\n", prefix, power, maxPower, fSoundPowerPercent);

	EmitSound_t ep;

	if (fSoundPowerPercent > 0.66f)
	{
		ep.name = varargs("%s_hard", prefix);
		ep.fVolume = fSoundPowerPercent;
	}
	if (fSoundPowerPercent > 0.33f)
	{
		ep.name = varargs("%s_medium", prefix);
		ep.fVolume = fSoundPowerPercent / 0.66f;
	}
	else
	{
		ep.name = varargs("%s_light", prefix);
		ep.fVolume = 0.8f;
	}

	ep.fPitch = RandomFloat(1.0f, 1.1f);
	ep.origin = origin;

	obj->EmitSoundWithParams(&ep);
}

//-------------------------------------------------------------------------------------------

BEGIN_NETWORK_TABLE( CObject_Physics )

	DEFINE_SENDPROP_FVECTOR3D(m_netPos),
	DEFINE_SENDPROP_FVECTOR3D(m_netAngles),
	
END_NETWORK_TABLE()

CObject_Physics::CObject_Physics( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_physBody = NULL;

}

CObject_Physics::~CObject_Physics()
{
	
}

void CObject_Physics::OnRemove()
{
	CGameObject::OnRemove();

	if(m_physBody)
	{
		g_pPhysics->m_physics.DestroyBody(m_physBody);
		m_physBody = NULL;
	}

	if(m_userData)
	{
		regobjectref_t* ref = (regobjectref_t*)m_userData;

		ref->object = NULL;
	}
}

void CObject_Physics::Spawn()
{
	SetModel( KV_GetValueString(m_keyValues->FindKeyBase("model"), 0, "models/error.egf") );

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_objectInstVertexFormat, g_pGameWorld->m_objectInstVertexBuffer );

		m_pModel->SetInstancer( instancer );
	}

	m_smashSound = KV_GetValueString(m_keyValues->FindKeyBase("smashsound"), 0, "");

	ses->PrecacheSound(m_smashSound.c_str());

	CEqRigidBody* body = new CEqRigidBody();

	BoundingBox bbox(m_pModel->GetBBoxMins(), m_pModel->GetBBoxMaxs());

	if( body->Initialize(&m_pModel->GetHWData()->m_physmodel, 0) )//
	{
		m_netPos = m_vecOrigin;
		m_netAngles = m_vecAngles;

		physobject_t* obj = &m_pModel->GetHWData()->m_physmodel.objects[0].object;

		body->SetFriction( 0.8f );
		body->SetRestitution( 0.0f );
		body->SetMass( obj->mass );
		body->SetDebugName("physics");

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_OBJECT );
		body->SetCollideMask( COLLIDEMASK_OBJECT );

		//body->SetCenterOfMass( obj->mass_center);

		body->m_flags = BODY_COLLISIONLIST | BODY_FROZEN;

		m_physBody = body;

		g_pPhysics->m_physics.AddToWorld( m_physBody );
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	//m_keyValues.Cleanup();

	// baseclass spawn
	CGameObject::Spawn();
}

void CObject_Physics::SetOrigin(const Vector3D& origin)
{
	if(m_physBody)
		m_physBody->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_Physics::SetAngles(const Vector3D& angles)
{
	if(m_physBody)
		m_physBody->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CObject_Physics::SetVelocity(const Vector3D& vel)
{
	if(!m_physBody)
		return;

	m_physBody->SetLinearVelocity( vel  );
}

const Vector3D& CObject_Physics::GetOrigin()
{
	m_vecOrigin = m_physBody->GetPosition();
	return m_vecOrigin;
}

const Vector3D& CObject_Physics::GetAngles()
{ 
	m_vecAngles = eulers(m_physBody->GetOrientation());
	m_vecAngles = VRAD2DEG(m_vecAngles);

	return m_vecAngles;
}

const Vector3D& CObject_Physics::GetVelocity() const
{
	return m_physBody->GetLinearVelocity();
}

extern ConVar r_enableObjectsInstancing;

void CObject_Physics::Draw( int nRenderFlags )
{
	if(!m_physBody)
		return;

	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(objBody->m_aabb.maxPoint)))
	//	return;

	m_physBody->UpdateBoundingBoxTransform();
	m_bbox = m_physBody->m_aabb_transformed;

	m_physBody->ConstructRenderMatrix(m_worldMatrix);

	if(r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		float camDist = g_pGameWorld->m_CameraParams.GetLODScaledDistFrom( GetOrigin() );
		int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();
		gameObjectInstance_t& inst = instancer->NewInstance( m_bodyGroupFlags, nLOD );
		inst.world = m_worldMatrix;
	}
	else
		CGameObject::Draw( nRenderFlags );
}

void CObject_Physics::Simulate(float fDt)
{
	PROFILE_FUNC()

	for(int i = 0; i < m_physBody->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = m_physBody->m_collisionList[i];

		m_physBody->SetMinFrameTime(0.0f);
	}

	if(	!m_physBody->IsFrozen() &&
		g_pGameSession->IsServer())
	{
		m_netPos.Set(m_physBody->GetPosition());
		m_netAngles.Set(GetAngles());

		m_vecOrigin = m_physBody->GetPosition();

		//OnNetworkStateChanged(NULL);
	}
}

void CObject_Physics::OnUnpackMessage( CNetMessageBuffer* buffer )
{
	BaseClass::OnUnpackMessage(buffer);

	debugoverlay->Box3D(m_netPos.Get()-1.0f, m_netPos.Get()+1.0f, ColorRGBA(1,1,0,0.5f), 0.0f );

	if(m_physBody)
	{
		m_physBody->SetPosition( m_netPos.Get() );
		m_vecOrigin = m_netPos.Get();
	}
	else
	{
		SetOrigin(m_netPos.Get());
	}

	SetAngles(m_netAngles.Get());
}

void CObject_Physics::L_SetContents(int contents)
{
	if (!m_physBody)
		return;

	m_physBody->SetContents(contents);
}

void CObject_Physics::L_SetCollideMask(int contents)
{
	if (!m_physBody)
		return;

	m_physBody->SetCollideMask(contents);
}

int	CObject_Physics::L_GetContents() const
{
	if (!m_physBody)
		return 0;

	return m_physBody->GetCollideMask();
}

int	CObject_Physics::L_GetCollideMask() const
{
	if (!m_physBody)
		return 0;

	return m_physBody->GetCollideMask();
}