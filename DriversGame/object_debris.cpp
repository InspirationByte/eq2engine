//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: debris object
//////////////////////////////////////////////////////////////////////////////////

#include "object_debris.h"
#include "materialsystem/IMaterialSystem.h"
#include "world.h"
#include "car.h"
#include "ParticleEffects.h"

#include "Shiny.h"

#define DEBRIS_COLLISION_RELAY	0.15f
#define DEBRIS_PHYS_LIFETIME	10.0f
#define DEBRIS_ERP				-0.02f

#define HUBCAP_ERP				0.025f

extern void EmitHitSoundEffect(CGameObject* obj, const char* prefix, const Vector3D& origin, float power, float maxPower);

CObject_Debris::CObject_Debris( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;

	m_hfObj = nullptr;
	m_fTimeToRemove = 0.0f;
	m_surfParams = nullptr;

	m_breakable = nullptr;
	m_breakSpawn = nullptr;

	m_smashSpawnedObject = nullptr;

	m_numBreakParts = 0;
}

CObject_Debris::~CObject_Debris()
{
	delete m_breakable;
	delete m_breakSpawn;
}

void CObject_Debris::OnRemove()
{
	BaseClass::OnRemove();

	if (m_hfObj)
	{
		g_pPhysics->RemoveObject(m_hfObj);
		m_hfObj = NULL;
	}

	if (m_smashSpawnedObject)
	{
		if(g_pGameWorld->IsValidObject(m_smashSpawnedObject))
			m_smashSpawnedObject->m_state = GO_STATE_REMOVE;
		m_smashSpawnedObject = nullptr;
	}
}

void CObject_Debris::Spawn()
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

	ASSERT(m_pModel);

	m_smashSound = KV_GetValueString(m_keyValues->FindKeyBase("smashSound"), 0, "");

	m_smashSpawn = KV_GetValueString(m_keyValues->FindKeyBase("smashSpawn"), 0, "");
	m_smashSpawnOffset = KV_GetVector3D(m_keyValues->FindKeyBase("smashSpawnOffset"), 0, vec3_zero);

	if(KV_GetValueBool(m_keyValues->FindKeyBase("castshadow"), 0, true))
		m_drawFlags |= GO_DRAW_FLAG_SHADOW;

	// init breakable
	kvkeybase_t* brkSec = m_keyValues->FindKeyBase("breakable");
	if(brkSec)
	{
		kvkeybase_t* brkPartsSec = brkSec->FindKeyBase("parts");

		if(brkPartsSec)
		{
			m_numBreakParts = brkPartsSec->keys.numElem();
			m_breakable = new breakablePart_t[m_numBreakParts];

			for(int i = 0; i < brkPartsSec->keys.numElem(); i++)
			{
				kvkeybase_t* partSec = brkPartsSec->keys[i];

				const char* partName = KV_GetValueString(partSec);

				m_breakable[i].bodyGroupIdx = Studio_FindBodyGroupId(m_pModel->GetHWData()->studio, partName);
				m_breakable[i].physObjIdx = PhysModel_FindObjectId(&m_pModel->GetHWData()->physModel, KV_GetValueString(partSec->FindKeyBase("physics"), 0, partName));

				m_breakable[i].offset = KV_GetVector3D(partSec->FindKeyBase("offset"));
			}
		}

		kvkeybase_t* spawnSec = brkSec->FindKeyBase("spawn");
		if(spawnSec)
		{
			m_breakSpawn = new breakSpawn_t;
			strcpy(m_breakSpawn->objectDefName, KV_GetValueString(spawnSec, 0, "bad_spawn_sec"));
			m_breakSpawn->offset = KV_GetVector3D(spawnSec->FindKeyBase("offset"));
		}
	}

	if(m_smashSound.Length() > 0)
	{
		g_sounds->PrecacheSound(m_smashSound.c_str());
		g_sounds->PrecacheSound((m_smashSound + "_light").c_str());
		g_sounds->PrecacheSound((m_smashSound + "_medium").c_str());
		g_sounds->PrecacheSound((m_smashSound + "_hard").c_str());
	}

	CEqRigidBody* body = new CEqRigidBody();

	if( body->Initialize(&m_pModel->GetHWData()->physModel, 0) )//
	{
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
			body->SetRestitution( 0.15f );
		}

		body->SetMass(obj->mass);
		body->SetGravity(body->GetGravity() * 2.0f);
		body->SetDebugName("hubcap");

		// additional error correction required
		body->m_erp = DEBRIS_ERP;

		//body->SetCenterOfMass( obj->mass_center);

		body->m_flags = COLLOBJ_DISABLE_RESPONSE | BODY_FORCE_FREEZE;

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_DEBRIS );
		body->SetCollideMask( COLLIDEMASK_DEBRIS );

		body->SetUserData(this);

		m_hfObj = new CPhysicsHFObject(body, this);
		g_pPhysics->AddObject(m_hfObj, false);

		m_bbox = body->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	// baseclass spawn
	BaseClass::Spawn();
}

void CObject_Debris::SpawnAsHubcap(IEqModel* model, int8 bodyGroup, int physObjectIdx)
{
	m_pModel = model;
	m_bodyGroupFlags = (1 << bodyGroup);

	m_drawFlags |= GO_DRAW_FLAG_SHADOW;

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init(g_worldGlobals.gameObjectVF, g_worldGlobals.objectInstBuffer );

		m_pModel->SetInstancer( instancer );
	}

	m_smashSound = "";

	CEqRigidBody* body = new CEqRigidBody();

	if( body->Initialize(&m_pModel->GetHWData()->physModel, physObjectIdx) )//
	{
		physobject_t* obj = &m_pModel->GetHWData()->physModel.objects[0].object;

		body->SetFriction( 0.7f );
		body->SetRestitution( 0.8f );
		body->SetMass(obj->mass);
		body->SetGravity(body->GetGravity() * 2.0f);
		body->SetDebugName("hubcap");

		// additional error correction required
		body->m_erp = HUBCAP_ERP;

		//body->SetCenterOfMass( obj->mass_center);

		body->m_flags = COLLOBJ_DISABLE_RESPONSE;

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_DEBRIS );
		body->SetCollideMask( COLLIDEMASK_DEBRIS );

		body->SetUserData(this);

		m_hfObj = new CPhysicsHFObject(body, this);
		g_pPhysics->AddObject(m_hfObj);

		m_bbox = body->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	// baseclass spawn
	BaseClass::Spawn();
}

void CObject_Debris::SpawnAsBreakablePart(IEqModel* model, int8 bodyGroup, int physObj)
{
	m_pModel = model;
	m_bodyGroupFlags = (1 << bodyGroup);

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init(g_worldGlobals.gameObjectVF, g_worldGlobals.objectInstBuffer );

		m_pModel->SetInstancer( instancer );
	}

	CEqRigidBody* body = new CEqRigidBody();

	if( body->Initialize(&m_pModel->GetHWData()->physModel, physObj) )//
	{
		physobject_t* obj = &m_pModel->GetHWData()->physModel.objects[physObj].object;

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
			body->SetRestitution( 0.15f );
		}

		body->SetFriction( 0.7f );
		body->SetRestitution( 0.8f );
		body->SetMass(obj->mass);
		body->SetGravity(body->GetGravity() * 2.0f);
		body->SetDebugName("brkbl");

		// additional error correction required
		body->m_erp = DEBRIS_ERP;

		body->m_flags = COLLOBJ_DISABLE_RESPONSE;

		body->SetPosition( m_vecOrigin );
		body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

		body->SetContents( OBJECTCONTENTS_DEBRIS );
		body->SetCollideMask( COLLIDEMASK_DEBRIS );

		body->SetUserData(this);

		m_hfObj = new CPhysicsHFObject(body, this);
		g_pPhysics->AddObject(m_hfObj);

		m_bbox = body->m_aabb_transformed;
	}
	else
	{
		MsgError("No physics model for '%s'\n", m_pModel->GetName());
		delete body;
	}

	// baseclass spawn
	BaseClass::Spawn();
}

void CObject_Debris::SetOrigin(const Vector3D& origin)
{
	if(m_hfObj)
		m_hfObj->GetBody()->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_Debris::SetAngles(const Vector3D& angles)
{
	if(m_hfObj)
		m_hfObj->GetBody()->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CObject_Debris::SetVelocity(const Vector3D& vel)
{
	if(m_hfObj)
	{
		CEqRigidBody* body = m_hfObj->GetBody();

		body->SetLinearVelocity(vel);
		body->TryWake();
	}
}

const Vector3D& CObject_Debris::GetOrigin() const
{
	return m_vecOrigin;
}

const Vector3D& CObject_Debris::GetAngles() const
{
	return m_vecAngles;
}

const Vector3D& CObject_Debris::GetVelocity() const
{
	return m_hfObj->GetBody()->GetLinearVelocity();
}

extern ConVar r_enableObjectsInstancing;

void CObject_Debris::Draw( int nRenderFlags )
{
	if(!m_hfObj)
		return;

	CEqRigidBody* body = m_hfObj->GetBody();

	// draw wheels
	if (!body->IsFrozen())
		m_shadowDecal.dirty = true;

	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(objBody->m_aabb.maxPoint)))
	//	return;

	if(IsSmashed())
	{
		body->UpdateBoundingBoxTransform();
		m_bbox = body->m_aabb_transformed;
	}

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

void CObject_Debris::BreakAndSpawnDebris(const CollisionPairData_t& coll)
{
	// remove this instance
	if(m_numBreakParts)
		m_state = GO_STATE_REMOVE;

	CEqRigidBody* body = m_hfObj->GetBody();

	for(int i = 0; i < m_numBreakParts; i++)
	{
		breakablePart_t& partDesc = m_breakable[i];

		// don't create unknown objects
		if(partDesc.bodyGroupIdx == -1 || partDesc.physObjIdx == -1)
			continue;

		CObject_Debris* part = new CObject_Debris(NULL);
		part->m_drawFlags = m_drawFlags;
		part->SpawnAsBreakablePart(GetModel(), partDesc.bodyGroupIdx, partDesc.physObjIdx);

		CEqRigidBody* newBody = part->m_hfObj->GetBody();

		// transform the offset
		Vector3D offset = m_worldMatrix*partDesc.offset;
		part->SetOrigin( GetOrigin() + offset );

		newBody->SetOrientation(body->GetOrientation());

		//Vector3D partVelocity = body->GetVelocityAtWorldPoint(coll.position);	
		newBody->SetLinearVelocity(body->GetLinearVelocity());
		newBody->SetAngularVelocity(body->GetAngularVelocity());

		// activate
		g_pGameWorld->AddObject(part);

		part->m_smashSound = m_smashSound;
	}

	m_numBreakParts = 0;
}

void CObject_Debris::OnPhysicsCollide(const CollisionPairData_t& pair)
{
	CEqRigidBody* body = m_hfObj->GetBody();
	eqPhysSurfParam_t* surf = m_surfParams;

	float impulse = pair.appliedImpulse * body->GetInvMass();

	if (surf && (surf->word == 'M') && pair.impactVelocity > 3.0f)
	{
		Vector3D wVelocity = body->GetVelocityAtWorldPoint(pair.position);
		Vector3D reflDir = reflect(wVelocity, pair.normal);

		MakeSparks(pair.position + pair.normal*0.05f, reflDir, Vector3D(5.0f), 1.0f, 6);
	}

	CEqCollisionObject* obj = pair.GetOppositeTo(body);
	EmitHitSoundEffect(this, m_smashSound.c_str(), pair.position, pair.impactVelocity, 50.0f);

	if (impulse > 10.0f)
	{
		if (m_numBreakParts > 0)
		{
			// make debris
			BreakAndSpawnDebris(pair);
		}
		else if (m_pModel->GetHWData()->studio->numBodyGroups > 1 && m_bodyGroupFlags == (1 << 0))
			m_bodyGroupFlags = (1 << 1);	// change look of model
	}

	bool moved = (body->m_flags & BODY_MOVEABLE) > 0;

	if (!moved && obj->IsDynamic())
	{
		if (m_smashSpawn.Length() > 0)
		{
			CGameObject* otherObject = g_pGameWorld->CreateObject(m_smashSpawn.c_str());
			if (otherObject)
			{
				otherObject->SetOrigin(GetOrigin() + m_smashSpawnOffset);
				otherObject->Spawn();
				g_pGameWorld->AddObject(otherObject);

				m_smashSpawnedObject = otherObject;
			}
		}

		moved = true;
		m_fTimeToRemove = 0.0f;

		g_pPhysics->m_physics.AddToMoveableList(body);
		body->Wake();

		CEqRigidBody* body = (CEqRigidBody*)obj;
		bool isCar = (body->m_flags & BODY_ISCAR) > 0;

		if (isCar)
		{
			EmitSound_t ep;

			ep.name = (char*)m_smashSound.c_str();

			ep.fPitch = RandomFloat(1.0f, 1.1f);
			ep.fVolume = 1.0f;
			ep.origin = pair.position;

			EmitSoundWithParams(&ep);
		}
	}
}

ConVar g_debris_as_physics("g_debris_as_physics", "0", "Thread debris as physics objects", CV_CHEAT);

void CObject_Debris::Simulate(float fDt)
{
	PROFILE_FUNC();

	if(fDt <= 0.0f)
		return;

	CEqRigidBody* body = m_hfObj->GetBody();
	bool moved = (body->m_flags & BODY_MOVEABLE) > 0;

	if(moved)
	{
		m_fTimeToRemove -= fDt;

		m_vecOrigin = body->GetPosition();
		Vector3D eulersAng = eulers(body->GetOrientation());
		m_vecAngles = VRAD2DEG(eulersAng);

		if(m_fTimeToRemove < 0.0f)
		{
			if(m_fTimeToRemove < -DEBRIS_PHYS_LIFETIME)
			{
				m_state = GO_STATE_REMOVE;
				return;
			}

			body->SetContents( OBJECTCONTENTS_DEBRIS );
			body->SetCollideMask( COLLIDEMASK_DEBRIS );

			body->m_flags &= ~BODY_FORCE_FREEZE;
		}
	}


	auto& collList = body->m_collisionList;

	// process collisions
	for(int i = 0; i < collList.numElem(); i++)
	{
		const CollisionPairData_t& pair = collList[i];

	}
}

bool CObject_Debris::IsSmashed() const
{ 
	return m_hfObj && (m_hfObj->GetBody()->m_flags & BODY_MOVEABLE) > 0; 
}

CEqRigidBody* CObject_Debris::GetPhysicsBody() const
{
	if (!m_hfObj)
		return NULL;
	return m_hfObj->GetBody();
}

void CObject_Debris::L_SetContents(int contents)
{
	if (!m_hfObj)
		return;

	m_hfObj->GetBody()->SetContents(contents);
}

void CObject_Debris::L_SetCollideMask(int contents)
{
	if (!m_hfObj)
		return;

	m_hfObj->GetBody()->SetCollideMask(contents);
}

int	CObject_Debris::L_GetContents() const
{
	if (!m_hfObj)
		return 0;

	return m_hfObj->GetBody()->GetCollideMask();
}

int	CObject_Debris::L_GetCollideMask() const
{
	if (!m_hfObj)
		return 0;

	return m_hfObj->GetBody()->GetCollideMask();
}
