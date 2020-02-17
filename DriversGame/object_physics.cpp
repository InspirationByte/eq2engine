//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: physics object
//////////////////////////////////////////////////////////////////////////////////

#include "object_physics.h"
#include "session_stuff.h"
#include "ParticleEffects.h"

#include "world.h"

#include "IDebugOverlay.h"

#include "state_game.h"

#include "Shiny.h"

void EmitHitSoundEffect(CGameObject* obj, const char* prefix, const Vector3D& origin, float power, float maxPower)
{
	if(*prefix == 0)
		return;

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
	m_hfObj = nullptr;
	m_surfParams = nullptr;
	m_userData = nullptr;
	m_breakSpawnedObject = nullptr;
	m_breakForce = 0.0f;
}

CObject_Physics::~CObject_Physics()
{

}

void CObject_Physics::OnRemove()
{
	BaseClass::OnRemove();

	if(m_hfObj)
	{
		g_pPhysics->RemoveObject(m_hfObj);
		m_hfObj = NULL;
	}

	if (m_breakSpawnedObject)
	{
		if (g_pGameWorld->IsValidObject(m_breakSpawnedObject))
			m_breakSpawnedObject->m_state = GO_STATE_REMOVE;

		m_breakSpawnedObject = nullptr;
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
		instancer->Init(g_worldGlobals.gameObjectVF, g_worldGlobals.objectInstBuffer );

		m_pModel->SetInstancer( instancer );
	}

	if (KV_GetValueBool(m_keyValues->FindKeyBase("castshadow"), 0, true))
		m_drawFlags |= GO_DRAW_FLAG_SHADOW;

	m_smashSound = KV_GetValueString(m_keyValues->FindKeyBase("smashsound"), 0, "");
	m_breakSpawn = KV_GetValueString(m_keyValues->FindKeyBase("breakspawn"), 0, "");
	m_breakSpawnOffset = KV_GetVector3D(m_keyValues->FindKeyBase("breakSpawnOffset"), 0, vec3_zero);
	m_breakForce = KV_GetValueFloat(m_keyValues->FindKeyBase("breakForce"), 0, 0.0f);

	g_sounds->PrecacheSound(m_smashSound.c_str());

	CEqRigidBody* body = new CEqRigidBody();

	if( body->Initialize(&m_pModel->GetHWData()->physModel, 0) )
	{
		m_hfObj = new CPhysicsHFObject(body, this);

		m_netPos = m_vecOrigin;
		m_netAngles = m_vecAngles;

		physobject_t* obj = &m_pModel->GetHWData()->physModel.objects[0].object;

		// set friction from surface parameters
		m_surfParams = g_pPhysics->FindSurfaceParam(obj->surfaceprops);
		if(m_surfParams)
		{
			body->SetFriction( m_surfParams->friction );
			body->SetRestitution( m_surfParams->restitution );
		}
		else
		{
			body->SetFriction( 0.8f );
			body->SetRestitution( 0.0f );
		}

		body->SetMass( obj->mass );
		body->SetDebugName("physics");

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_OBJECT );
		body->SetCollideMask( COLLIDEMASK_OBJECT );

		body->SetUserData(this);

		//body->SetCenterOfMass( obj->mass_center);

		body->m_flags = COLLOBJ_COLLISIONLIST | BODY_FROZEN;

		if (m_breakForce > 0.0f)
			body->m_flags |= BODY_FORCE_FREEZE;

		g_pPhysics->AddObject( m_hfObj );
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	//m_keyValues.Cleanup();

	// baseclass spawn
	BaseClass::Spawn();
}

void CObject_Physics::SetOrigin(const Vector3D& origin)
{
	if(m_hfObj)
		m_hfObj->GetBody()->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_Physics::SetAngles(const Vector3D& angles)
{
	if(m_hfObj)
		m_hfObj->GetBody()->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CObject_Physics::SetVelocity(const Vector3D& vel)
{
	if(!m_hfObj)
		return;

	m_hfObj->GetBody()->SetLinearVelocity( vel  );
}

const Vector3D& CObject_Physics::GetVelocity() const
{
	return m_hfObj->GetBody()->GetLinearVelocity();
}

extern ConVar r_enableObjectsInstancing;

void CObject_Physics::Draw( int nRenderFlags )
{
	if(!m_hfObj)
		return;

	CEqRigidBody* body = m_hfObj->GetBody();

	// draw wheels
	if (!body->IsFrozen())
		m_shadowDecal.dirty = true;

	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(objBody->m_aabb.maxPoint)))
	//	return;

	body->UpdateBoundingBoxTransform();
	m_bbox = body->m_aabb_transformed;

	body->ConstructRenderMatrix(m_worldMatrix);

	if(r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		float camDist = g_pGameWorld->m_view.GetLODScaledDistFrom( GetOrigin() );
		int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();

		for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
		{
			if(!(m_bodyGroupFlags & (1 << i)))
				continue;

			gameObjectInstance_t& inst = instancer->NewInstance( i , nLOD );
			inst.world = m_worldMatrix;
		}

	}
	else
		BaseClass::Draw( nRenderFlags );
}

void CObject_Physics::OnPhysicsCollide(CollisionPairData_t& pair)
{
	CEqRigidBody* body = m_hfObj->GetBody();

	if (!body->IsFrozen())
		body->SetMinFrameTime(0.0f);

	if (m_surfParams && m_surfParams->word == 'M' && pair.impactVelocity > 4.0f)
	{
		Vector3D wVelocity = body->GetVelocityAtWorldPoint(pair.position);
		Vector3D reflDir = reflect(wVelocity, pair.normal);
		MakeSparks(pair.position + pair.normal*0.05f, reflDir, Vector3D(5.0f), 1.0f, 8);
	}

	// it's only broken if car collision force was greateer
	if (pair.impactVelocity > m_breakForce)
	{
		if (m_breakForce > 0.0f)
			m_hfObj->GetBody()->Wake();

		if (!m_breakSpawnedObject && m_breakSpawn.Length() > 0)
		{
			CGameObject* otherObject = g_pGameWorld->CreateObject(m_breakSpawn.c_str());
			if (otherObject)
			{
				otherObject->SetOrigin(GetOrigin() + m_breakSpawnOffset);
				otherObject->Spawn();
				g_pGameWorld->AddObject(otherObject);

				m_breakSpawnedObject = otherObject;
			}
		}
	}
}

void CObject_Physics::Simulate(float fDt)
{
	PROFILE_FUNC();

	if(fDt <= 0.0f)
		return;


}

void CObject_Physics::OnUnpackMessage( CNetMessageBuffer* buffer )
{
	BaseClass::OnUnpackMessage(buffer);

	debugoverlay->Box3D(m_netPos.Get()-1.0f, m_netPos.Get()+1.0f, ColorRGBA(1,1,0,0.5f), 0.0f );

	if(m_hfObj)
	{
		m_hfObj->GetBody()->SetPosition( m_netPos.Get() );
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
	if (!m_hfObj)
		return;

	m_hfObj->GetBody()->SetContents(contents);
}

void CObject_Physics::L_SetCollideMask(int contents)
{
	if (!m_hfObj)
		return;

	m_hfObj->GetBody()->SetCollideMask(contents);
}

int	CObject_Physics::L_GetContents() const
{
	if (!m_hfObj)
		return 0;

	return m_hfObj->GetBody()->GetCollideMask();
}

int	CObject_Physics::L_GetCollideMask() const
{
	if (!m_hfObj)
		return 0;

	return m_hfObj->GetBody()->GetCollideMask();
}
