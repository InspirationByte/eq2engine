//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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
	m_physBody = NULL;
	m_surfParams = NULL;
	m_userData = NULL;
	m_hinge = NULL;
	m_hingeDummy = NULL;
}

CObject_Physics::~CObject_Physics()
{

}

void CObject_Physics::OnRemove()
{
	BaseClass::OnRemove();

	if(m_hingeDummy)
	{
		g_pPhysics->m_physics.DestroyController(m_hinge);

		g_pPhysics->m_physics.DestroyBody(m_hingeDummy);
		m_hingeDummy = NULL;
	}

	if(m_physBody)
	{
		g_pPhysics->m_physics.DestroyBody(m_physBody);
		m_physBody = NULL;
	}

	// this might be required to
	if(m_userData)
	{
		regionObject_t* ref = (regionObject_t*)m_userData;
		ref->game_object = NULL;
		m_userData = NULL;
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
		instancer->Init(g_worldGlobals.objectInstFormat, g_worldGlobals.objectInstBuffer );

		m_pModel->SetInstancer( instancer );
	}

	if (KV_GetValueBool(m_keyValues->FindKeyBase("castshadow"), 0, true))
		m_drawFlags |= GO_DRAW_FLAG_SHADOW;

	m_smashSound = KV_GetValueString(m_keyValues->FindKeyBase("smashsound"), 0, "");

	g_sounds->PrecacheSound(m_smashSound.c_str());

	CEqRigidBody* body = new CEqRigidBody();

	if( body->Initialize(&m_pModel->GetHWData()->physModel, 0) )
	{
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

		m_physBody = body;

		g_pPhysics->m_physics.AddToWorld( m_physBody );

		kvkeybase_t* hingeKvs = m_keyValues->FindKeyBase("hinge");
		if(hingeKvs)
		{
			Vector3D hingePoint = KV_GetVector3D(hingeKvs);

			m_hingeDummy = new CEqRigidBody();
			m_hingeDummy->Initialize(1.0f);
			m_hingeDummy->SetMass( 32000.0f );
			m_hingeDummy->Freeze();
			m_hingeDummy->SetPosition(GetOrigin() + hingePoint + vec3_up*0.15f);
			m_hingeDummy->SetContents(0);
			m_hingeDummy->SetCollideMask(0);

			g_pPhysics->m_physics.AddToWorld(m_hingeDummy);

			m_hinge = new CEqPhysicsHingeJoint();

			m_hinge->Init(body, m_hingeDummy, vec3_up, hingePoint, 0.2f, 10.0f, 10.0f, 0.1f, 0.005f);

			g_pPhysics->m_physics.AddController( m_hinge );
		}
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

	// draw wheels
	if (!m_physBody->IsFrozen())
		m_shadowDecal.dirty = true;

	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(objBody->m_aabb.maxPoint)))
	//	return;

	m_physBody->UpdateBoundingBoxTransform();
	m_bbox = m_physBody->m_aabb_transformed;

	m_physBody->ConstructRenderMatrix(m_worldMatrix);

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

void CObject_Physics::Simulate(float fDt)
{
	PROFILE_FUNC();

	if(fDt <= 0.0f)
		return;

	float maxImpactVelocity = 0.0f;

	for(int i = 0; i < m_physBody->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = m_physBody->m_collisionList[i];
		m_physBody->SetMinFrameTime(0.0f);

		if(m_surfParams && m_surfParams->word == 'M' && pair.impactVelocity > 4.0f)
		{
			Vector3D wVelocity = m_physBody->GetVelocityAtWorldPoint( pair.position );
			Vector3D reflDir = reflect(wVelocity, pair.normal);
			MakeSparks(pair.position+pair.normal*0.05f, reflDir, Vector3D(5.0f), 1.0f, 8);
		}

		CEqCollisionObject* collidingObject = pair.GetOppositeTo(m_physBody);

		// hinge is only broken if car collision force was greateer
		if(m_hinge && (collidingObject->m_flags & BODY_ISCAR))
		{
			maxImpactVelocity = max(maxImpactVelocity, pair.impactVelocity);
		}
	}

	if(	!m_physBody->IsFrozen() &&
		g_pGameSession->IsServer())
	{
		if(m_hinge && maxImpactVelocity > 0.0f && maxImpactVelocity < 1.0f)
		{
			m_hinge->SetEnabled(true);
		}

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
