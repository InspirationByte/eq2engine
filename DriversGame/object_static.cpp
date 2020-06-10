//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: static object
//////////////////////////////////////////////////////////////////////////////////

#include "object_static.h"
#include "materialsystem/IMaterialSystem.h"
#include "world.h"
#include "ParticleEffects.h"

#include "Shiny.h"

CObject_Static::CObject_Static( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_physObj = NULL;
	m_flicker = false;
	m_killed = false;
	m_flickerTime = 0;
}

CObject_Static::~CObject_Static()
{

}

void CObject_Static::OnRemove()
{
	BaseClass::OnRemove();

	if(m_physObj)
	{
		m_physObj->SetUserData(NULL);
		g_pPhysics->m_physics.DestroyStaticObject(m_physObj);
	}
	m_physObj = NULL;
}

void CObject_Static::Spawn()
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

	studioPhysData_t& physData = m_pModel->GetHWData()->physModel;

	if (physData.numObjects)
	{
		m_physObj = new CEqCollisionObject();

		if (m_physObj->Initialize(&physData, 0))
		{
			physobject_t* obj = &m_pModel->GetHWData()->physModel.objects[0].object;

			// deny wheel and camera collisions
			m_physObj->m_flags = COLLOBJ_NO_RAYCAST;

			m_physObj->SetPosition(m_vecOrigin);
			m_physObj->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x), DEG2RAD(m_vecAngles.y), DEG2RAD(m_vecAngles.z)));
			m_physObj->SetUserData(this);

			m_physObj->SetContents(OBJECTCONTENTS_SOLID_OBJECTS);
			m_physObj->SetCollideMask(0);

			// set friction from surface parameters
			eqPhysSurfParam_t* surfParams = g_pPhysics->FindSurfaceParam(obj->surfaceprops);

			if (surfParams)
			{
				// friction is reduced
				m_physObj->SetFriction(surfParams->friction * 0.25f);
				m_physObj->SetRestitution(surfParams->restitution);
			}

			g_pPhysics->m_physics.AddStaticObject(m_physObj);

			m_bbox = m_physObj->m_aabb_transformed;

			m_physObj->ConstructRenderMatrix(m_worldMatrix);
		}
		else
		{
			MsgError("No physics model for '%s'\n", m_pModel->GetName());
			delete m_physObj;
		}
	}

	LoadDefLightData(m_light, m_keyValues);

	// baseclass spawn
	BaseClass::Spawn();
}

void CObject_Static::SetOrigin(const Vector3D& origin)
{
	if(m_physObj)
		m_physObj->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_Static::SetAngles(const Vector3D& angles)
{
	if(m_physObj)
		m_physObj->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CObject_Static::SetVelocity(const Vector3D& vel)
{

}

extern ConVar r_enableObjectsInstancing;

void CObject_Static::Draw( int nRenderFlags )
{
	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(m_pModel->GetBBoxMaxs())))
	//	return;

	if(m_physObj)
		m_physObj->ConstructRenderMatrix(m_worldMatrix);

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

void CObject_Static::Simulate(float fDt)
{
	BaseClass::Simulate(fDt);

	PROFILE_FUNC();

	if((g_pGameWorld->m_envConfig.lightsType & WLIGHTS_LAMPS) && !m_killed)
	{
		float flickerVal = 1.0f;

		if(m_flicker)
		{
			m_flickerTime += fDt;

			flickerVal = fabs(sin(sin(m_flickerTime*10.0f)*2.5f))*0.5f+0.5f;
		}

		DrawDefLightData(m_worldMatrix, m_light, flickerVal);
	}
}

void CObject_Static::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	if(m_killed)
		return;

	if(pair.bodyA->IsDynamic())
	{
		CEqRigidBody* body = (CEqRigidBody*)pair.bodyA;

		float impulse = pair.appliedImpulse*body->GetInvMass();

		if(impulse > 5.0f)
			m_flicker = true;

		if(impulse > 15.0f)
		{
			m_killed = true;

			if(!(g_pGameWorld->m_envConfig.lightsType & WLIGHTS_LAMPS))
				return;

			for(int i = 0; i < m_light.m_glows.numElem(); i++)
			{
				// transform light position
				Vector3D lightPos = m_light.m_glows[i].position.xyz();
				lightPos = (m_worldMatrix*Vector4D(lightPos, 1.0f)).xyz();

				MakeSparks(lightPos, Vector3D(0,-10,0), Vector3D(55.0f), 1.2f ,25);
			}
		}
	}
}

void CObject_Static::L_SetContents(int contents)
{
	if (!m_physObj)
		return;

	m_physObj->SetContents(contents);
}

void CObject_Static::L_SetCollideMask(int contents)
{
	if (!m_physObj)
		return;

	m_physObj->SetCollideMask(contents);
}

int	CObject_Static::L_GetContents() const
{
	if (!m_physObj)
		return 0;

	return m_physObj->GetCollideMask();
}

int	CObject_Static::L_GetCollideMask() const
{
	if (!m_physObj)
		return 0;

	return m_physObj->GetCollideMask();
}