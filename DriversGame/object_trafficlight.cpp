//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Traffic light
//////////////////////////////////////////////////////////////////////////////////

#include "object_trafficlight.h"
#include "ParticleEffects.h"
#include "world.h"

#include "Shiny.h"

ConVar g_debugTrafficLight("g_debug_trafficlight", "0", NULL, CV_CHEAT);

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
	m_physObj = NULL;
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
	BaseClass::OnRemove();

	if(m_physObj)
	{
		m_physObj->SetUserData(NULL);
		g_pPhysics->m_physics.DestroyStaticObject(m_physObj);
	}
	m_physObj = NULL;
}

void CObject_TrafficLight::Spawn()
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

	m_physObj = new CEqCollisionObject();

	if( m_physObj->Initialize(&m_pModel->GetHWData()->physModel, 0) )//
	{
		physobject_t* obj = &m_pModel->GetHWData()->physModel.objects[0].object;

		// set friction from surface parameters
		eqPhysSurfParam_t* surfParams = g_pPhysics->FindSurfaceParam(obj->surfaceprops);
		if(surfParams)
		{
			m_physObj->SetFriction( surfParams->friction );
			m_physObj->SetRestitution( surfParams->restitution );
		}
		else
		{
			m_physObj->SetFriction( 0.8f );
			m_physObj->SetRestitution( 0.0f );
		}

		// deny wheel and camera collisions
		m_physObj->m_flags = COLLOBJ_NO_RAYCAST | COLLOBJ_SINGLE_CONTACT;

		m_physObj->SetPosition( m_vecOrigin );
		m_physObj->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));
		m_physObj->SetUserData(this);

		m_physObj->SetContents( OBJECTCONTENTS_SOLID_OBJECTS );
		m_physObj->SetCollideMask( 0 );

		g_pPhysics->m_physics.AddStaticObject( m_physObj );

		m_bbox = m_physObj->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete m_physObj;
	}

	Matrix4x4 objectMat;
	m_physObj->ConstructRenderMatrix(objectMat);

	for(int i = 0; i < m_keyValues->keys.numElem(); i++)
	{
		if(!stricmp(m_keyValues->keys[i]->name, "trafficlight"))
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

	BaseClass::Spawn();
}

void CObject_TrafficLight::SetOrigin(const Vector3D& origin)
{
	if(m_physObj)
		m_physObj->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_TrafficLight::SetAngles(const Vector3D& angles)
{
	if(m_physObj)
		m_physObj->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;

	m_trafficDir = GetDirectionIndexByAngles(m_vecAngles);
}

void CObject_TrafficLight::SetVelocity(const Vector3D& vel)
{

}

extern ConVar r_enableObjectsInstancing;

void CObject_TrafficLight::Draw( int nRenderFlags )
{
	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(m_pModel->GetBBoxMaxs())))
	//	return;

	if( g_debugTrafficLight.GetBool() )
	{
		IVector2D cellPos = g_pGameWorld->m_level.PositionToGlobalTilePoint(GetOrigin());
		int laneRowDir = GetPerpendicularDir(m_trafficDir);

		int dx[4] = ROADNEIGHBOUR_OFFS_X(0);
		int dy[4] = ROADNEIGHBOUR_OFFS_Y(0);

		IVector2D forwardDir = IVector2D(dx[m_trafficDir],dy[m_trafficDir]);
		IVector2D rightDir = IVector2D(dx[laneRowDir],dy[laneRowDir]);

		Vector3D cellOrigin = g_pGameWorld->m_level.GlobalTilePointToPosition(cellPos);

		debugoverlay->Box3D(cellOrigin-1,cellOrigin+1, ColorRGBA(1,1,0,1));

		Vector3D cellDirOrigin = g_pGameWorld->m_level.GlobalTilePointToPosition(cellPos + forwardDir);
		Vector3D cellLeftOrigin = g_pGameWorld->m_level.GlobalTilePointToPosition(cellPos + rightDir);

		debugoverlay->Box3D(cellDirOrigin-0.25f,cellDirOrigin+0.25f, ColorRGBA(0,0,1,1));
		debugoverlay->Box3D(cellLeftOrigin-0.25f,cellLeftOrigin+0.25f, ColorRGBA(1,0,0,1));

		IVector2D bestCell;
		if(g_pGameWorld->m_level.Road_FindBestCellForTrafficLight(bestCell, GetOrigin(), m_trafficDir))
		{
			Vector3D bestCellPos = g_pGameWorld->m_level.GlobalTilePointToPosition(bestCell);

			debugoverlay->Box3D(bestCellPos-2,bestCellPos+2, ColorRGBA(0,1,1,1));
		}
	}

	//-------------------------------------------

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

		Vector3D dir;
		AngleVectors(m_vecAngles-Vector3D(0.0f,90.0f,0.0f), &dir);

		Vector3D cam_pos = g_pGameWorld->m_view.GetOrigin();
		Vector3D cam_forward;
		AngleVectors(g_pGameWorld->m_view.GetAngles(), &cam_forward);

		Plane pl(dir,-dot(dir, m_vecOrigin+dir*5.0f));

		float fAngFade = clamp( pl.Distance(cam_pos), 0.0f, 8.0f) * 0.125f;

		int drawnLightType = 0;
		float greenBlinker = 1.0f;
		bool drawOrange = false;

		float globalTrafficLightTime = g_pGameWorld->m_globalTrafficLightTime;

		// green light - blinks if remaining time less than 6 seconds, then yellow light shown when less than 2 seconds
		if(g_pGameWorld->m_globalTrafficLightDirection == trafDir)
		{
			drawOrange = (globalTrafficLightTime < 2.0f);

			bool blinkGreen = (globalTrafficLightTime < 6.0f) && !drawOrange;

			drawnLightType = 2;

			if(blinkGreen)
				greenBlinker = clamp(sinf(globalTrafficLightTime*7.0f)*100.0f, 0.0f, 1.0f);
			else if(drawOrange)
				drawnLightType = -1;
		}
		else // red light - shown always and yellow shows when time is less than 3 seconds
		{
			drawOrange = (globalTrafficLightTime < 3.0f);
			drawnLightType = 0;
		}


		if( CheckVisibility(g_pGameWorld->m_occludingFrustum) )
		{
			for(int i = 0; i < m_lights.numElem(); i++)
			{
				trafficlights_t tl = m_lights[i];

				if(tl.type == 0 && drawnLightType == 0)
				{
					DrawLightEffect(tl.position, ColorRGBA(lightColorTypes[tl.type]*flickerVal, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE, 1);
					DrawLightEffect(tl.position, ColorRGBA(lightColorTypes[tl.type]*flickerVal*0.5f, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE*2.0f, 2);
				}
				else if(tl.type == 2 && drawnLightType == 2)
				{
					DrawLightEffect(tl.position, ColorRGBA(lightColorTypes[tl.type]*flickerVal, 1.0f) * fAngFade * greenBlinker, TRAFFICLIGHT_GLOW_SIZE, 1);
					DrawLightEffect(tl.position, ColorRGBA(lightColorTypes[tl.type]*flickerVal*0.5f, 1.0f) * fAngFade * greenBlinker, TRAFFICLIGHT_GLOW_SIZE*2.0f, 2);
				}
				else if(tl.type == 1 && drawOrange)
				{
					DrawLightEffect(tl.position, ColorRGBA(lightColorTypes[tl.type]*flickerVal, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE, 1);
					DrawLightEffect(tl.position, ColorRGBA(lightColorTypes[tl.type]*flickerVal*0.5f, 1.0f) * fAngFade, TRAFFICLIGHT_GLOW_SIZE*2.0f, 2);
				}
			} // for
		} // check
	}
}

void CObject_TrafficLight::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	if(m_killed)
		return;

	if(pair.bodyA->IsDynamic())
	{
		CEqRigidBody* body = (CEqRigidBody*)pair.bodyA;

		float impulse = pair.appliedImpulse*body->GetInvMass();

		if(impulse > 5.0f)
		{
			m_flicker = true;
		}

		if(impulse > 15.0f)
		{
			m_killed = true;

			for(int i = 0; i < m_lights.numElem(); i++)
			{
				// transform light position
				Vector3D lightPos = m_lights[i].position;

				MakeSparks(lightPos, Vector3D(0,-10,0), Vector3D(75.0f), 1.2f, 7);
			}
		}
	}
}
