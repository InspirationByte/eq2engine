//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Traffic light
//////////////////////////////////////////////////////////////////////////////////

#include "object_trafficlight.h"
#include "materialsystem/IMaterialSystem.h"
#include "world.h"

#include "Shiny.h"

static ColorRGB lightColorTypes[] = 
{
	ColorRGB(1.0f, 0.145f, 0.13f),	// red
	ColorRGB(1.0f, 0.6f, 0.0f),		// orange
	ColorRGB(0, 0.915f, 0.643f),	// green
};

#define TRAFFICLIGHT_GLOW_SIZE		(0.37f)

CObject_TrafficLight::CObject_TrafficLight( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_pPhysicsObject = NULL;
	m_flicker = false;
	m_killed = false;
	m_flickerTime = 0;
	m_trafficDir = 0;
}

CObject_TrafficLight::~CObject_TrafficLight()
{

}

void CObject_TrafficLight::OnRemove()
{
	CGameObject::OnRemove();

	if(m_pPhysicsObject)
	{
		m_pPhysicsObject->SetUserData(NULL);
		g_pPhysics->m_physics.DestroyStaticObject(m_pPhysicsObject);
	}
	m_pPhysicsObject = NULL;

	if(m_userData)
	{
		regionObject_t* ref = (regionObject_t*)m_userData;
		ref->game_object = NULL;
	}
}

void CObject_TrafficLight::Spawn()
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

	m_pPhysicsObject = new CEqCollisionObject();

	if( m_pPhysicsObject->Initialize(&m_pModel->GetHWData()->m_physmodel, 0) )//
	{
		physobject_t* obj = &m_pModel->GetHWData()->m_physmodel.objects[0].object;

		// deny wheel and camera collisions
		m_pPhysicsObject->m_flags = COLLOBJ_NO_RAYCAST;

		m_pPhysicsObject->SetPosition( m_vecOrigin );
		m_pPhysicsObject->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));
		m_pPhysicsObject->SetUserData(this);

		m_pPhysicsObject->SetContents( OBJECTCONTENTS_SOLID_OBJECTS );
		m_pPhysicsObject->SetCollideMask( 0 );

		g_pPhysics->m_physics.AddStaticObject( m_pPhysicsObject );

		m_bbox = m_pPhysicsObject->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete m_pPhysicsObject;
	}

	Matrix4x4 objectMat;
	m_pPhysicsObject->ConstructRenderMatrix(objectMat);

	for(int i = 0; i < m_keyValues->keys.numElem(); i++)
	{
		if(!stricmp(m_keyValues->keys[i]->name, "light"))
		{
			trafficlights_t light;
			light.position = KV_GetVector3D(m_keyValues->keys[i]);
			light.type = KV_GetValueInt(m_keyValues->keys[i], 3);
			light.direction = KV_GetValueInt(m_keyValues->keys[i], 4);

			light.type = clamp(light.type, 0,2);

			// transform light position
			Vector3D lightPos = light.position;
			light.position = (objectMat*Vector4D(lightPos, 1.0f)).xyz();

			m_lights.append(light);
		}
	}

	//m_keyValues.Cleanup();
	// baseclass spawn
	CGameObject::Spawn();
}

void CObject_TrafficLight::SetOrigin(const Vector3D& origin)
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_TrafficLight::SetAngles(const Vector3D& angles)
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;

	m_trafficDir = 0;

	float fAng = m_vecAngles.y;

	if(fAng > 0)
	{
		while(fAng > 89.0f)
		{
			m_trafficDir++;
			fAng -= 90.0f;
		}
	}
	else
	{
		while(fAng < -89.0f)
		{
			m_trafficDir++;
			fAng += 90.0f;
		}
	}
}

void CObject_TrafficLight::SetVelocity(const Vector3D& vel)
{

}

extern ConVar r_enableObjectsInstancing;

void CObject_TrafficLight::Draw( int nRenderFlags )
{
	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(m_pModel->GetBBoxMaxs())))
	//	return;

	m_pPhysicsObject->ConstructRenderMatrix(m_worldMatrix);

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

//
// from car.cpp, pls move
//
extern void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type = 0);

void CObject_TrafficLight::Simulate(float fDt)
{
	PROFILE_FUNC();

	if(!m_killed)
	{
		float flickerVal = 1.0f;

		if(m_flicker)
		{
			m_flickerTime += fDt;

			flickerVal = fabs(sin(sin(m_flickerTime*10.0f)*2.5f))*0.5f+0.5f;
		}

		int trafDir = m_trafficDir % 2;

		//if(trafDir > 2)
		//	trafDir -= 2;

		Vector3D dir;
		AngleVectors(m_vecAngles-Vector3D(0.0f,90.0f,0.0f), &dir);

		Vector3D cam_pos = g_pGameWorld->m_CameraParams.GetOrigin();
		Vector3D cam_forward;
		AngleVectors(g_pGameWorld->m_CameraParams.GetAngles(), &cam_forward);

		Plane pl(dir,-dot(dir, m_vecOrigin+dir*5.0f));

		float fAngFade = clamp( pl.Distance(cam_pos), 0.0f, 8.0f) * 0.125f;

		int drawnLightType = 0;

		if( g_pGameWorld->m_globalTrafficLightDirection == trafDir )
			drawnLightType = 2;

		bool drawOrange = (g_pGameWorld->m_globalTrafficLightTime < 3.0f);

		if(drawnLightType == 0 && drawOrange)
			drawnLightType = 1;

		float remainderVal = 1.0f;

		if(g_pGameWorld->m_globalTrafficLightDirection == trafDir && drawOrange)
			remainderVal *= clamp(sin(g_pGameWorld->m_globalTrafficLightTime*7.0f)*100.0f, 0.0f, 1.0f);

		if( CheckVisibility(g_pGameWorld->m_occludingFrustum) )
		{
			for(int i = 0; i < m_lights.numElem(); i++)
			{
				if(m_lights[i].type == 0 && drawnLightType == 0)
				{
					DrawLightEffect(m_lights[i].position, ColorRGBA(lightColorTypes[m_lights[i].type]*flickerVal, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE, 1);
					DrawLightEffect(m_lights[i].position, ColorRGBA(lightColorTypes[m_lights[i].type]*flickerVal*0.5f, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE*2.0f, 2);
				}
				else if(m_lights[i].type == 2 && drawnLightType == 2)
				{
					DrawLightEffect(m_lights[i].position, ColorRGBA(lightColorTypes[m_lights[i].type]*flickerVal, 1.0f) * fAngFade * remainderVal, TRAFFICLIGHT_GLOW_SIZE, 1);
					DrawLightEffect(m_lights[i].position, ColorRGBA(lightColorTypes[m_lights[i].type]*flickerVal*0.5f, 1.0f) * fAngFade * remainderVal, TRAFFICLIGHT_GLOW_SIZE*2.0f, 2);
				}
				else if(m_lights[i].type == 1 && drawOrange)
				{
					DrawLightEffect(m_lights[i].position, ColorRGBA(lightColorTypes[m_lights[i].type]*flickerVal, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE, 1);
					DrawLightEffect(m_lights[i].position, ColorRGBA(lightColorTypes[m_lights[i].type]*flickerVal*0.5f, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE*2.0f, 2);
				}
			} // for
		} // check
	}
}

void CObject_TrafficLight::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	if(pair.bodyA->IsDynamic())
	{
		CEqRigidBody* body = (CEqRigidBody*)pair.bodyA;

		float impulse = pair.appliedImpulse*body->GetInvMass();

		if(impulse > 28.0f)
		{
			m_flicker = true;
		}
	
		if(impulse > 48.0f)
		{
			m_killed = true;
		}
	}
}