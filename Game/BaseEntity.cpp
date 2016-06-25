//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base entity class
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEntity.h"
#include "DebugInterface.h"
#include "EntityQueue.h"
#include "IDebugOverlay.h"
#include "GameRenderer.h"

ConVar ph_maxPhysicsEventDistance("ph_maxPhysicsEventDistance", "512", "Maximum physics event distance from camera\n", CV_ARCHIVE);

// declare save data
BEGIN_DATAMAP_NO_BASE(BaseEntity)

	DEFINE_KEYFIELD( m_szClassName, "classname", VTYPE_STRING),

	DEFINE_KEYFIELD( m_pszTargetName, "name", VTYPE_STRING),
	DEFINE_KEYFIELD( m_pszTargetName, "targetname", VTYPE_STRING),

	DEFINE_KEYFIELD( m_vecAbsOrigin, "origin", VTYPE_ORIGIN),
	//DEFINE_KEYFIELD( m_vecAbsAngles[1], "angle", VTYPE_ANGLE),
	DEFINE_KEYFIELD( m_vecAbsAngles, "angles", VTYPE_ANGLES),
	DEFINE_KEYFIELD( m_vecLightingOrigin,	"lightorigin", VTYPE_VECTOR3D),

	DEFINE_FIELD( m_vecOrigin, VTYPE_ORIGIN),
	DEFINE_FIELD( m_vecAngles,  VTYPE_ANGLES),

	DEFINE_FIELD( m_matWorldTransform, VTYPE_MATRIX4X4),

	DEFINE_KEYFIELD( m_pszModelName, "model", VTYPE_MODELNAME),
	DEFINE_KEYFIELD( m_pszScriptName, "script", VTYPE_STRING),

	DEFINE_FIELD( m_pPhysicsObject, VTYPE_PHYSICSOBJECT),

	DEFINE_KEYFIELD( m_pszTargetEntity, "target", VTYPE_STRING),
	DEFINE_FIELD( m_pTargetEntityPointer, VTYPE_ENTITYPTR),

	//DEFINE_FIELD( m_nNextThinkTick, VTYPE_INTEGER),

	// owner entity
	DEFINE_FIELD( m_pOwner, VTYPE_ENTITYPTR),
	DEFINE_FIELD( m_nEntIndex, VTYPE_INTEGER), // we need to save entity index to restore the VTYPE_ENTITYPTR

	DEFINE_FIELD( m_vecAbsVelocity, VTYPE_VECTOR3D), // only velocity is not key/value

	DEFINE_FIELD( m_nHealth, VTYPE_INTEGER),
	DEFINE_FIELD( m_nMaxHealth, VTYPE_INTEGER),

	DEFINE_FIELD( m_fnThink, VTYPE_FUNCTION),
	DEFINE_FIELD( m_fNextThink, VTYPE_TIME),
	DEFINE_FIELD( m_bThinked, VTYPE_BOOLEAN),
	
	DEFINE_OUTPUT("OnMapSpawn", m_OnMapSpawn),
	DEFINE_OUTPUT("OnRemove", m_OnRemove),
	DEFINE_OUTPUT("OnUse", m_OnUse),

	DEFINE_INPUTFUNC("Attach", InputAttach),
	DEFINE_INPUTFUNC("Kill", InputKill),

	DEFINE_INPUTFUNC("ScriptFunc", InputScriptFunc), // TODO: script execution by inputs/outputs

END_DATAMAP()

void BaseEntity::Activate()
{
	// if we spawned with this state, immideately remove this entity
	if(GetState() == ENTITY_REMOVE)
		return;

	if(!m_bRegistered)
	{
		SetState( ENTITY_IDLE );
		UniqueRegisterEntityForUpdate(this);
		m_bRegistered = true;
	}
}

BaseEntity::BaseEntity()
{
	m_vecAbsOrigin			= vec3_zero;
	m_vecAbsAngles			= vec3_zero;
	m_vecAbsVelocity		= vec3_zero;

	m_vecLightingOrigin		= vec3_zero;

	m_vecOrigin				= vec3_zero;
	m_vecAngles				= vec3_zero;
	//m_vecVelocity			= vec3_zero;

	m_vBBox.AddVertex(Vector3D(-10));
	m_vBBox.AddVertex(Vector3D(10));

	m_szClassName			= "failed";

	m_fNextThink			= 0;
	m_fnThink				= NULL;
	m_bThinked				= true;

	m_pPhysicsType			= PHYSTYPE_STATIC;
	m_pPhysicsObject		= NULL;

	m_pModel				= NULL;

	m_bRegistered			= false;

	m_pszTargetEntity		= "";
	m_pszTargetName			= "";
	m_pszModelName			= "";

	m_nEntityState			= ENTITY_NO_UPDATE;

	m_pOwner				= NULL;

	m_nEntIndex				= -1;

	m_pTargetEntityPointer	= NULL;

	m_nMaxHealth			= 1;
	m_nHealth				= 1;

	//m_pTableObject			= NULL;
	//m_pUserObject			= NULL;

	m_matWorldTransform		= identity4();

	// render stuff
	memset(m_static_lights, 0, sizeof(m_static_lights));
	m_nRooms				= -1;
	m_vLastPos				= MAX_COORD_UNITS;

	// create table object
	//EQGMS_INIT_OBJECT();
}

extern DkList<BaseEntity*> *ents;

void BaseEntity::SetIndex(int newidx)
{
	BaseEntity* ent_from_idx = (BaseEntity*)UTIL_EntByIndex( newidx );

	if(ent_from_idx)// swap
	{
		ents->ptr()[newidx] = this;
		ents->ptr()[m_nEntIndex] = ent_from_idx;
		ent_from_idx->m_nEntIndex = m_nEntIndex;
	}
	else // add
	{
		ents->insert(this, newidx);
	}

	m_nEntIndex = newidx;
}

int BaseEntity::EntIndex()
{
	return m_nEntIndex;
}

void BaseEntity::OnPostRender()
{

}

// called when game starts
void BaseEntity::OnGameStart()
{
	variable_t var;

	m_OnGameStart.FireOutput(var, this, this, 0.0f);
	m_OnGameStart.Clear();

	/*
	// do it
	OBJ_BEGINCALL("OnGameStart")
	{
		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;
	*/
}

void BaseEntity::InitScriptHooks()
{
	// load script if present
	if(m_pszScriptName.Length())
	{
		// you can use ENT_BINDSCRIPT in your other operations
		//OBJ_BINDSCRIPT( m_pszScriptName.GetData(), this );
	}
}

// precaches entity
void BaseEntity::Precache()
{
	// it precaches script and pre-executes here
	InitScriptHooks();

	/*
	// do it
	OBJ_BEGINCALL("OnPrecache")
	{
		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;*/
}

void BaseEntity::Spawn()
{
	UpdateRenderStuff();

	/*
	// do it
	OBJ_BEGINCALL("OnSpawn")
	{
		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;
	*/
}

// sets new target entity
void BaseEntity::SetTargetEntity(BaseEntity* pEnt) 
{
	if(!pEnt)
	{
		m_pTargetEntityPointer = NULL;
		m_pszTargetEntity = "";
		return;
	}
		
	if(m_pTargetEntityPointer)
	{
		if(stricmp(m_pszTargetEntity.GetData(), m_pTargetEntityPointer->GetName()))
		{
			m_pTargetEntityPointer = pEnt; 
			m_pszTargetEntity = pEnt->GetName();
		}
	}
	else
	{
		m_pTargetEntityPointer = pEnt; 
		m_pszTargetEntity = pEnt->GetName();
	}
}

void BaseEntity::CheckTargetEntity()
{
	if(m_pszTargetEntity.Length() > 0)
	{
		m_pTargetEntityPointer = (BaseEntity*)UTIL_EntByName(m_pszTargetEntity.GetData());

		if(!m_pTargetEntityPointer)
			MsgWarning("Warning! Target object '%s' not found in world!\n", m_pszTargetEntity.GetData());
	}
}

void BaseEntity::Simulate(float decaytime)
{
	if(StateUpdate())
		Update( decaytime );
}

bool BaseEntity::StateUpdate()
{
	switch(GetState())
	{
		case ENTITY_IDLE:
			return true;
		case ENTITY_NO_UPDATE:
			return false;
		case ENTITY_REMOVE:
			return false;
	}
	return true;
}

void BaseEntity::ThinkSet(BASE_THINK_PTR fnThink)
{
	m_fnThink = fnThink;
	m_bThinked = false;
}

void BaseEntity::UpdateOnRemove()
{
	// Call removal functions
	OnRemove();

	/*
	// do it
	OBJ_BEGINCALL("OnRemove")
	{
		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;*/
}

void BaseEntity::Think()
{
	/*
	// do it
	OBJ_BEGINCALL("OnThink")
	{
		call.End();
	}
	OBJ_CALLDONE;*/

	// Let derived objects think if they want to
	if ( m_fnThink )
	{
		(this->*m_fnThink)();
	}
}

void BaseEntity::ThinkFrame()
{
	if(gpGlobals->curtime >= m_fNextThink && !m_bThinked)
	{
		m_bThinked = true;

		//int nTicks = currTimeTicks - m_nNextThinkTick;
		//Msg("Think ticks to do: %d\n", nTicks);
		
		Think();
	}
}

void BaseEntity::Update(float decaytime)
{
	// think for objects
	ThinkFrame();

	UpdateTransform();

	/*
	// update physics object event radially
	if(PhysicsGetObject() && !(PhysicsGetObject()->GetFlags() & PO_NO_EVENTS) && !(PhysicsGetObject()->GetFlags() & PO_NO_EVENT_BLOCK))
	{
		Vector3D mainCameraPos = vec3_zero;

		if(viewrenderer->GetView())
			mainCameraPos = viewrenderer->GetView()->GetOrigin();

		Vector3D vv = m_vecAbsOrigin-mainCameraPos;

		float fDist = length(vv);

		if(fDist > ph_maxPhysicsEventDistance.GetFloat())
			PhysicsGetObject()->AddFlags(PO_BLOCKEVENTS);
		else
			PhysicsGetObject()->RemoveFlags(PO_BLOCKEVENTS);
	}
	*/
}

// checks object for usage ray/box
bool BaseEntity::CheckUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir)
{
	return false;
}

// onUse handler
bool BaseEntity::OnUse(BaseEntity* pActivator, const Vector3D& origin, const Vector3D& dir)
{
	variable_t var;

	m_OnUse.FireOutput(var, pActivator, this, 0.0f);

	/*
	// do it
	OBJ_BEGINCALL("OnUse")
	{
		call.AddParamUser(pActivator->GetScriptUserObject());
		call.AddParamUser(gmVector3_Create(GetScriptSys()->GetMachine(), origin));
		call.AddParamUser(gmVector3_Create(GetScriptSys()->GetMachine(), dir));

		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;
	*/

	return true;
}

// min bbox dimensions
Vector3D BaseEntity::GetBBoxMins()
{
	return m_vBBox.minPoint;
}

// max bbox dimensions
Vector3D BaseEntity::GetBBoxMaxs()
{
	return m_vBBox.maxPoint;
}

// renders model
void BaseEntity::Render(int nViewRenderFlags)
{
	RenderEGFModel( nViewRenderFlags, NULL );
}

ConVar r_drawEntities("r_drawEntities", "1", "Draw entities", CV_CHEAT);
ConVar r_waterModelLod("r_waterModelLod", "1", 0, 10, "Model water lod", CV_ARCHIVE);

// renders EGF model
// client code
void BaseEntity::RenderEGFModel( int nViewRenderFlags, Matrix4x4* bones )
{
	if(!r_drawEntities.GetBool())
		return;

	if(!m_pModel)
		return;

	materials->SetMatrix(MATRIXMODE_WORLD, m_matWorldTransform);

	BoundingBox bbox;

	// slower but more efficient
	for(int i = 0; i < 9; i++)
		bbox.AddVertex( (m_matWorldTransform*Vector4D(m_vBBox.GetVertex(i), 1.0f)).xyz() );	

	Volume frustum = *viewrenderer->GetViewFrustum();

	Vector3D mins = bbox.minPoint;
	Vector3D maxs = bbox.maxPoint;

	// check frustum visibility
	if(!(nViewRenderFlags & VR_FLAG_ALWAYSVISIBLE) && !frustum.IsBoxInside(mins.x,maxs.x,mins.y,maxs.y,mins.z,maxs.z))
		return;

	//
	// Room culling - check visibility
	//
	if( viewrenderer->GetAreaList() && viewrenderer->GetView() && !(nViewRenderFlags & VR_FLAG_ALWAYSVISIBLE) )
	{
		Vector3D bbox_center = (mins+maxs)*0.5f;
		float bbox_size = length(mins-maxs);

		UpdateRenderStuff();

		int view_rooms[2] = {-1, -1,};
		viewrenderer->GetViewRooms(view_rooms);

		int cntr = 0;

		for(int i = 0; i < m_nRooms; i++)
		{
			if(m_pRooms[i] != -1)
			{
				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
				{
					cntr++;
					continue;
				}

				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(m_vecAbsOrigin, bbox_size))
				{
					cntr++;
					continue;
				}
			}
		}

		if(cntr == m_nRooms)
			return;
	}

	if((nViewRenderFlags & VR_FLAG_ALWAYSVISIBLE))
		UpdateRenderStuff();

	// we've need to reset skinning
	viewrenderer->SetModelInstanceChanged();

	Vector3D view_vec = g_pViewEntity->GetEyeOrigin() - m_matWorldTransform.getTranslationComponent();

	studiohdr_t* pHdr = m_pModel->GetHWData()->pStudioHdr;

	// select the LOD
	int nLOD = m_pModel->SelectLod( length(view_vec) ); // lod distance check

	// add water reflection lods
	if(nViewRenderFlags & VR_FLAG_WATERREFLECTION)
		nLOD += r_waterModelLod.GetInt();

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		// TODO: check bodygroups for rendering

		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[ nLOD ];

		// get the right LOD model number
		while(nLOD > 0 && nModDescId == -1)
		{
			nLOD--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[ nLOD ];
		}

		if(nModDescId == -1)
			continue;

		// render model groups that in this body group
		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			if( bones )
				materials->SetSkinningEnabled(true);

			// slow part of rendering
			viewrenderer->DrawModelPart(m_pModel, nModDescId, j, nViewRenderFlags, m_static_lights, bones);
		}
	}

	/*
	// render decals only on two first lods
	if(nLOD < 2)
	{
		for(int i = 0; i < m_pDecals.numElem(); i++)
		{
			// transform verts and render
			
		}
	}
	*/
}

#define UPDATE_VIS_LIGHTS_DIST_SQR (128.0f)

// updates rendering stuff
void BaseEntity::UpdateRenderStuff()
{
	UpdateTransform();

	Vector3D origin = transpose(m_matWorldTransform).getTranslationComponent() + Vector3D(0.0f,5.0f,0.0f) + m_vecLightingOrigin;
	
	// TODO: trace for lighting origin

	Vector3D checkVec = m_vLastPos - origin;

	if(dot(checkVec, checkVec) > UPDATE_VIS_LIGHTS_DIST_SQR)
	{
		m_vLastPos = origin;
		m_nRooms = eqlevel->GetRoomsForSphere( origin+m_vBBox.GetCenter(), length(m_vBBox.GetSize()), m_pRooms );

		viewrenderer->GetNearestEightLightsForPoint(origin, m_static_lights);
	}
}

// returns owner (parent entity)
BaseEntity*	BaseEntity::GetOwner()
{
	return m_pOwner;
}

// sets parent entity
void BaseEntity::SetOwner(BaseEntity *pOwner)
{
	m_pOwner = pOwner;
}

// fills matrix with parent transformation
void BaseEntity::GetParentToWorldTransform( Matrix4x4& matrix )
{
	if(!m_pOwner)
	{
		matrix = identity4();
		return;
	}

	GetOwner()->UpdateTransform();

	Matrix4x4 parentMatrix;
	GetOwner()->GetWorldTransform(parentMatrix);

	Matrix4x4 childMatrix;
	GetWorldTransform(childMatrix);

	matrix = transpose(!childMatrix*parentMatrix);
}

void BaseEntity::GetWorldTransform( Matrix4x4& matrix )
{
	matrix = m_matWorldTransform;
}

void BaseEntity::UpdateTransform()
{
	if( GetOwner() )
	{
		// update owner
		GetOwner()->UpdateTransform();
		
		// get parent transformation
		Matrix4x4 parentTransform;
		GetOwner()->GetWorldTransform( parentTransform );

		// make local rotation matrix
		Matrix4x4 rotation = rotateXYZ4(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z));

		// make rotation to get valid placenent
		Matrix4x4 attachMatrix = parentTransform*rotation;

		// update absolute position with local origin
		m_vecAbsOrigin = (attachMatrix*Vector4D(m_vecOrigin, 1)).xyz();
		Vector3D ang = EulerMatrixXYZ(attachMatrix.getRotationComponent());

		// setup absolute angles
		m_vecAbsAngles = VRAD2DEG(ang);
		
		// reset owner if removes
		if(GetOwner()->GetState() == ENTITY_REMOVE)
			SetOwner(NULL);
	}
	else
	{
		m_vecOrigin = m_vecAbsOrigin;
		m_vecAngles = m_vecAbsAngles;
	}

	ComputeTransformMatrix();
}

// compute matrix transformation for this object
void BaseEntity::ComputeTransformMatrix()
{
	m_matWorldTransform = ComputeWorldMatrix(GetAbsOrigin(), GetAbsAngles(), Vector3D(1));
}

// sets absolute position of this entity
void BaseEntity::SetAbsOrigin(const Vector3D &pos)
{
	m_vecAbsOrigin = pos;

	Vector3D newOrigin;

	if(m_pOwner)
	{
		Matrix4x4 parent_transform;
		m_pOwner->GetWorldTransform(parent_transform);

		newOrigin = (!parent_transform*Vector4D(pos, 1)).xyz();
	}
	else
		newOrigin = pos;

	m_vecOrigin = newOrigin;
}

// sets absolute angles of this entity
void BaseEntity::SetAbsAngles(const Vector3D &ang)
{
	m_vecAbsAngles = ang;

	Vector3D newAngles;

	if(m_pOwner)
	{
		Matrix4x4 parentTransform;
		m_pOwner->GetWorldTransform( parentTransform );

		// make local rotation matrix
		Matrix4x4 rotation = rotateXYZ4(DEG2RAD(ang.x),DEG2RAD(ang.y),DEG2RAD(ang.z));

		// make rotation to get valid placenent
		Matrix4x4 attachMatrix = !parentTransform*rotation;

		Vector3D angles = EulerMatrixXYZ( parentTransform.getRotationComponent() );
		newAngles = VRAD2DEG( angles );
	}
	else
		newAngles = ang;

	m_vecAngles = newAngles;
}

// sets absolute velocity of this entity
void BaseEntity::SetAbsVelocity(const Vector3D &vel)
{
	m_vecAbsVelocity = vel;
}

// returns absolute position
Vector3D BaseEntity::GetAbsOrigin()
{
	return m_vecAbsOrigin;
}

// returns absolute angles
Vector3D BaseEntity::GetAbsAngles()
{
	return m_vecAbsAngles;
}

// returns absolute velocity
Vector3D BaseEntity::GetAbsVelocity()
{
	return m_vecAbsVelocity;
}

// sets absolute position of this entity
void BaseEntity::SetLocalOrigin(const Vector3D &pos)
{
	m_vecOrigin = pos;
}

// sets absolute angles of this entity
void BaseEntity::SetLocalAngles(const Vector3D &ang)
{
	m_vecAngles = ang;
}

// returns absolute position
Vector3D BaseEntity::GetLocalOrigin()
{
	return m_vecOrigin;
}

// returns absolute angles
Vector3D BaseEntity::GetLocalAngles()
{
	return m_vecAngles;
}

// sets new classname of this entity
void BaseEntity::SetClassName(const char* newclassname)
{
	m_szClassName = newclassname;
}

// sets target name of this entity
void BaseEntity::SetName(const char* newname)
{	
	m_pszTargetName = newname;
}

// returns current class name of this entity
const char*	BaseEntity::GetClassname()
{
	return (char*)m_szClassName.GetData();
}

// returns target name
const char*	BaseEntity::GetName()
{
	return (char*)m_pszTargetName.GetData();
}

void BaseEntity::SetNextThink(float flTime)
{
	m_fNextThink = flTime;//( flTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS( flTime );
	//Msg("BaseEntity::SetNextThink : set to %d ticks, current time in ticks is: %d\n", m_nNextThinkTick, TIME_TO_TICKS(gpGlobals->curtime));
	m_bThinked = false;
}

const char* BaseEntity::GetModelName()
{
	if(m_pszModelName.Length() == 0)
		return NULL;

	return m_pszModelName.GetData();
}

// sets model for this entity
void BaseEntity::SetModel(IEqModel* pModel)
{
	m_pModel = pModel;

	ClearDecals();

	if(m_pModel)
		m_vBBox = m_pModel->GetAABB();
}

void BaseEntity::SetModel(const char* pszModelName)
{
	if(!pszModelName)
	{
		m_pModel = NULL;

		m_pszModelName = "";

		return;
	}

	if(!m_pszModelName.CompareCaseIns(pszModelName) && m_pModel)
		return;

	if(strlen(pszModelName) == 0)
	{
		//SetModel( (IEqModel*)NULL );
		return;
	}

	m_pszModelName = pszModelName;

	int mod_index = g_pModelCache->GetModelIndex(pszModelName);

	if(mod_index == -1)
		MsgError("Model %s is not cached\n", pszModelName);

	IEqModel* pModel = g_pModelCache->GetModel(mod_index);

	SetModel( pModel );
}

IPhysicsObject* BaseEntity::PhysicsInitStatic()
{
	PhysicsDestroyObject();

	if(m_pModel && m_pModel->GetHWData()->m_physmodel.numobjects == 1)
	{
		float fOldMass = m_pModel->GetHWData()->m_physmodel.objects[0].object.mass;
		m_pModel->GetHWData()->m_physmodel.objects[0].object.mass = 0;
		IPhysicsObject* pObject = physics->CreateObject(&m_pModel->GetHWData()->m_physmodel);

		pObject->SetEntityObjectPtr(this);

		// disable events for this object
		pObject->AddFlags( PO_NO_EVENTS );

		m_pModel->GetHWData()->m_physmodel.objects[0].object.mass = fOldMass;

		PhysicsSetObject(pObject);

		return pObject;
	}
	else
	{
		//Msg("Physics object count: %d\n", m_pModel->GetHWData()->m_physmodel.numobjects);
	}

	return NULL;
}

IPhysicsObject* BaseEntity::PhysicsInitNormal(int nSolidFlags, bool makeAsleep, int objectIndex)
{
#pragma fixme("This could work wrong")

	PhysicsDestroyObject();

	if(m_pModel->GetHWData()->m_physmodel.numobjects == 1)
	{
		objectIndex = 0;
	}
	else
	{
		if(m_pModel->GetHWData()->m_physmodel.numobjects > 1 && objectIndex == -1)
		{
			MsgWarning("Too many physics objects in POD for model %s (%d)\n", m_pModel->GetName(), m_pModel->GetHWData()->m_physmodel.numobjects);
			objectIndex = 0;
		}
	}

	IPhysicsObject* pObject = physics->CreateObject(&m_pModel->GetHWData()->m_physmodel, objectIndex);

	PhysicsSetObject(pObject);

	pObject->SetContents(nSolidFlags);

	if(!makeAsleep)
		pObject->WakeUp();
	else
		pObject->SetActivationState( PS_INACTIVE );

	pObject->SetEntityObjectPtr(this);

	return pObject;
}

IPhysicsObject* BaseEntity::PhysicsInitCustom(int nSolidFlags, bool makeAsleep, int numShapes, int* shapeIdxs, const char* surfProps, float mass)
{
	IPhysicsObject* pObject = physics->CreateObjectCustom(numShapes, shapeIdxs, surfProps, mass);

	PhysicsSetObject(pObject);

	pObject->SetContents(nSolidFlags);

	if(!makeAsleep)
		pObject->WakeUp();
	else
		pObject->SetActivationState( PS_INACTIVE );

	pObject->SetEntityObjectPtr(this);

	return pObject;
}

void BaseEntity::PhysicsDestroyObject()
{
	if(m_pPhysicsObject)
	{
		physics->DestroyPhysicsObject(m_pPhysicsObject);
		PhysicsSetObject(NULL);
	}
}

void BaseEntity::OnRemove()
{
	variable_t val;
	m_OnRemove.FireOutput(val, this, this, 0.0f);

	PhysicsDestroyObject();

	//GetScriptSys()->FreeTableObject(m_pTableObject);

	//if(m_pUserObject)
	//	GetScriptSys()->FreeUserObject(m_pUserObject);

	ClearDecals();
}

void BaseEntity::OnMapSpawn()
{
	variable_t var;

	m_OnMapSpawn.FireOutput(var, this, this, 0.0f);
	m_OnMapSpawn.Clear();

	/*
	// do it
	OBJ_BEGINCALL("OnMapSpawn")
	{
		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;
	*/
}

EntRelClass_e BaseEntity::Classify()
{
	return ENTCLASS_NONE;
}

// entity type for filtering
EntType_e BaseEntity::EntType()
{
	return ENTTYPE_GENERIC;
}

// entity processing type for engine perfomance
EntProcType_e BaseEntity::GetProcessingType()
{
	return ENTPROC_GLOBAL;
}

void BaseEntity::SetHealth(int nHealth)
{
	m_nHealth = nHealth;
	
	if(m_nHealth > m_nMaxHealth)
		m_nHealth = m_nMaxHealth;

	if(m_nHealth < 0)
		m_nHealth = 0;
}

int BaseEntity::GetHealth()
{
	return m_nHealth;
}

bool BaseEntity::IsDead()
{
	return m_nHealth <= 0;
}

bool BaseEntity::IsAlive()
{
	return m_nHealth > 0;
}

// removes decals
void BaseEntity::ClearDecals()
{
	// destroy decals
	for(int i = 0; i < m_pDecals.numElem(); i++)
	{
		delete m_pDecals[i];
	}
}

// applyes damage to entity (use this)
void BaseEntity::MakeDecal( const decalmakeinfo_t& info )
{
	if( m_pModel )
	{
		decalmakeinfo_t dec_info = info;
		dec_info.origin = (!m_matWorldTransform * Vector4D(dec_info.origin, 1.0f)).xyz();

		Matrix3x3 rotation = m_matWorldTransform.getRotationComponent();
		dec_info.normal = !rotation * dec_info.normal;

		studiotempdecal_t* pStudioDecal = m_pModel->MakeTempDecal( dec_info, NULL);
		m_pDecals.append(pStudioDecal);
	}
}

void BaseEntity::ApplyDamage( const damageInfo_t& info )
{
	/*
	// do it
	OBJ_BEGINCALL("OnApplyDamage")
	{
		//call.AddParamTable(damageInfoTable);

		call.AddParam(gmVariable(info.m_fDamage));
		call.AddParam(gmVariable(info.m_nDamagetype));
		call.AddParam(info.m_pInflictor ? gmVariable(info.m_pInflictor->GetScriptUserObject()) : gmVariable(GM_NULL,0));

		// push values and smth
		call.End();

		// get values and smth
	}
	OBJ_CALLDONE;*/

	SetHealth(GetHealth() - info.m_fDamage);
}

// entity view - computation for rendering
// use SetActiveViewEntity to set view from this entity
void BaseEntity::CalcView(Vector3D &vOrigin, Vector3D &vAngles, float &fFov)
{
	vOrigin = GetAbsOrigin();
	vAngles = GetAbsAngles();
	fFov = 90;
}

Vector3D BaseEntity::GetEyeOrigin()
{
	return GetAbsOrigin();
}

Vector3D BaseEntity::GetEyeAngles()
{
	return GetAbsAngles();
}

void BaseEntity::AcceptInput(char* pszInputName, variable_t &Value, BaseEntity *pActivator, BaseEntity *pCaller)
{
	//Msg("Input from '%s', function '%s', value is '%s'\n", pCaller->GetClassname(), pszInputName, Value.GetString());

	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the actions in the data description, looking for a match
		for ( int i = 0; i < dmap->m_dataNumFields; i++ )
		{
			if ( dmap->m_fields[i].nFlags & FIELDFLAG_INPUT )
			{
				if(!stricmp( dmap->m_fields[i].name, pszInputName ))
				{
					inputfn_t pfnInput = dmap->m_fields[i].functionPtr;

					inputdata_t data;
					data.value = Value;
					data.id = 0;
					data.pActivator = pActivator;
					data.pCaller = pCaller;
					
					// call it
					(this->*pfnInput)(data);
				}
			}
		}
	}
}

int splitstring_singlecharseparator(char* str, char separator, DkList<EqString> &outStrings)
{
	char c = str[0];

	char* iterator = str;

	char* pFirst = str;
	char* pLast = NULL;

	while(c != 0)
	{
		c = *iterator;

		if(c == separator || c == 0)
		{
			pLast = iterator;

			int char_count = pLast - pFirst;

			if(char_count > 0)
			{
				// add new string
				outStrings.append(EqString(pFirst, 0, char_count));
			}

			pFirst = iterator+1;
		}

		iterator++;
	}

	return outStrings.numElem();
}

// sets key
void BaseEntity::SetKey(const char* pszName, const char* pszValue)
{
	//Msg("Set key %s=%s for %s\n", pszName, pszValue, GetClassname());
	SetKeyDatamap(pszName, pszValue, GetDataDescMap());
}

void BaseEntity::SetKey(const char* pszName, const variable_t& value)
{
	SetKeyDatamap(pszName, value, GetDataDescMap());
}

// returns key value
variable_t BaseEntity::GetKey(const char* pszName)
{
	return GetKeyDatamap(pszName, GetDataDescMap());
}

variable_t BaseEntity::GetKeyDatamap(const char* pszName, datamap_t* pDataMap)
{
	for(int k = 0; k < pDataMap->m_dataNumFields; k++)
	{
		if(!stricmp(pDataMap->m_fields[k].name, pszName))
		{
			return GetDataVariableOf(pDataMap->m_fields[k]);
		}
	}

	if(pDataMap->baseMap != NULL)
		return GetKeyDatamap( pszName, pDataMap->baseMap);

	variable_t empty;
	empty.SetInt(0);

	return empty;
}

variable_t BaseEntity::GetDataVariableOf(const datavariant_t& var)
{
	variable_t value;

	if(	var.type == VTYPE_STRING ||
		var.type == VTYPE_MODELNAME ||
		var.type == VTYPE_SOUNDNAME)
	{
		EqString* strPtr = (EqString*)((char *)this + var.offset);
		value.SetString(strPtr->GetData());
	}
	else if(var.type == VTYPE_FLOAT || var.type == VTYPE_TIME)
	{
		float* ptr = (float*)((char *)this + var.offset);
		value.SetFloat(*ptr);
	}
	else if(var.type == VTYPE_INTEGER)
	{
		int* ptr = (int*)((char *)this + var.offset);
		value.SetInt(*ptr);
	}
	else if(var.type == VTYPE_BOOLEAN)
	{
		bool* ptr = (bool*)((char *)this + var.offset);
		value.SetBool(*ptr);
	}

	else if(var.type == VTYPE_SHORT)
	{
		short* ptr = (short*)((char *)this + var.offset);
		value.SetInt(*ptr);
	}

	else if(var.type == VTYPE_BYTE)
	{
		int8* ptr = (int8*)((char *)this + var.offset);
		value.SetInt(*ptr);
	}

	else if(var.type == VTYPE_VECTOR2D)
	{
		Vector2D* vec = (Vector2D *)((char *)this + var.offset);

		value.SetVector2D(*vec);
	}

	else if(var.type == VTYPE_VECTOR3D)
	{
		Vector3D* vec = (Vector3D *)((char *)this + var.offset);
		value.SetVector3D(*vec);
	}

	else if(var.type == VTYPE_VECTOR4D)
	{
		Vector4D* vec = (Vector4D *)((char *)this + var.offset);
		value.SetVector4D(*vec);
	}

	else if(var.type == VTYPE_ORIGIN ||
		var.type == VTYPE_ANGLES)
	{
		Vector3D* vec = (Vector3D *)((char *)this + var.offset);
		value.SetVector3D(*vec);	
	}

	else if(var.type == VTYPE_ANGLE)
	{
		float* ptr = (float*)((char *)this + var.offset);
		value.SetFloat(*ptr);
	}

	else if(var.type == VTYPE_MATRIX2X2)
	{
		Matrix2x2* vec = (Matrix2x2 *)((char *)this + var.offset);
		value.SetMatrix2x2(*vec);
	}

	else if(var.type == VTYPE_MATRIX3X3)
	{
		Matrix3x3* vec = (Matrix3x3 *)((char *)this + var.offset);
		value.SetMatrix3x3(*vec);
	}

	else if(var.type == VTYPE_MATRIX4X4)
	{
		Matrix4x4* vec = (Matrix4x4 *)((char *)this + var.offset);
		value.SetMatrix4x4(*vec);
	}

	return value;
}

void BaseEntity::SetKeyDatamap(const char* pszName, const variable_t& var, datamap_t* pDataMap)
{
	variable_t value = var;

	for(int k = 0; k < pDataMap->m_dataNumFields; k++)
	{
		if(!stricmp(pDataMap->m_fields[k].name, pszName))
		{
			if(	pDataMap->m_fields[k].type == VTYPE_STRING ||
				pDataMap->m_fields[k].type == VTYPE_MODELNAME ||
				pDataMap->m_fields[k].type == VTYPE_SOUNDNAME)
			{
				EqString* strPtr = (EqString*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_STRING);
				*strPtr = value.GetString();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_FLOAT || pDataMap->m_fields[k].type == VTYPE_TIME)
			{
				float* ptr = (float*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_FLOAT);
				*ptr = value.GetFloat();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_FUNCTION)
			{
				void** function_ptr = (void**)((char *)this + pDataMap->m_fields[k].offset);
				// TODO: find function by reg. name
				//ptr = 

				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_INTEGER)
			{
				int* ptr = (int*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_INTEGER);
				*ptr = value.GetInt();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_BOOLEAN)
			{
				bool* ptr = (bool*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_BOOLEAN);
				*ptr = value.GetBool();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_SHORT)
			{
				short* ptr = (short*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_INTEGER);
				*ptr = (short)value.GetInt();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_BYTE)
			{
				int8* ptr = (int8*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_INTEGER);
				*ptr = (int8)value.GetInt();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_VECTOR2D)
			{
				Vector2D* vec = (Vector2D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector2D decode;
				value.Convert(VTYPE_VECTOR2D);

				decode = value.GetVector2D();

				vec->x = decode.x;
				vec->y = decode.y;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_VECTOR3D)
			{
				Vector3D* vec = (Vector3D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector3D decode;
				value.Convert(VTYPE_VECTOR3D);

				decode = value.GetVector3D();

				vec->x = decode.x;
				vec->y = decode.y;
				vec->z = decode.z;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_VECTOR4D)
			{
				Vector4D* vec = (Vector4D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector4D decode;
				value.Convert(VTYPE_VECTOR4D);

				decode = value.GetVector4D();

				vec->x = decode.x;
				vec->y = decode.y;
				vec->z = decode.z;
				vec->w = decode.w;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_ORIGIN ||
				pDataMap->m_fields[k].type == VTYPE_ANGLES)
			{
				Vector3D* vec = (Vector3D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector3D decode;
				value.Convert(VTYPE_VECTOR3D);

				decode = value.GetVector3D();

				vec->x = decode.x;
				vec->y = decode.y;
				vec->z = decode.z;

				//Msg("Angles parsed for %s (%g %g %g)\n", pEnt->GetClassname(), decode.x, decode.y, decode.z);
					
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_ANGLE)
			{
				float* ptr = (float*)((char *)this + pDataMap->m_fields[k].offset);
				value.Convert(VTYPE_FLOAT);
				*ptr = value.GetFloat();
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_MATRIX2X2)
			{
				Matrix2x2* vec = (Matrix2x2 *)((char *)this + pDataMap->m_fields[k].offset);
				Matrix2x2 decode;
				value.Convert(VTYPE_MATRIX2X2);

				decode = value.GetMatrix2x2();

				*vec = decode;
				return;
			}

			if(pDataMap->m_fields[k].type == VTYPE_MATRIX3X3)
			{
				Matrix3x3* vec = (Matrix3x3 *)((char *)this + pDataMap->m_fields[k].offset);

				Matrix3x3 decode;

				value.Convert(VTYPE_MATRIX3X3);

				decode = value.GetMatrix3x3();

				Matrix3x3 final;
				final.rows[0] = decode.rows[0]; // z forward
				final.rows[1] = decode.rows[1]; // x
				final.rows[2] = decode.rows[2]; // y 

				*vec = final;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_MATRIX4X4)
			{
				Matrix4x4* vec = (Matrix4x4 *)((char *)this + pDataMap->m_fields[k].offset);
				Matrix4x4 decode;
				value.Convert(VTYPE_MATRIX4X4);

				decode = value.GetMatrix4x4();

				*vec = decode;
				return;
			}
		}
	}

	if(pDataMap->baseMap != NULL)
		SetKeyDatamap( pszName, var, pDataMap->baseMap);
}

// sets key
void BaseEntity::SetKeyDatamap(const char* pszName, const char* pszValue, datamap_t* pDataMap)
{
	if(!stricmp( pszName,"classname" ))
		return;

	// add output events
	if(!stricmp(pszName, "EntityOutput"))
	{
		DkList<EqString> strings;

		splitstring_singlecharseparator((char*)pszValue, ';', strings);

		for(int outp = 0; outp < strings.numElem(); outp++)
		{
			char* pszBegin = (char*)strings[outp].GetData();

			while(*pszBegin != ',')
				pszBegin++;

			int len = (pszBegin - strings[outp].GetData());

			char* pszText = (char*)stackalloc(len);
			strncpy(pszText, strings[outp].GetData(), len);
			pszText[len] = '\0';

			for(int k = 0; k < pDataMap->m_dataNumFields; k++)
			{
				if((pDataMap->m_fields[k].nFlags & FIELDFLAG_OUTPUT) && !stricmp(pDataMap->m_fields[k].name, pszText))
				{
					CBaseEntityOutput* pOutput = (CBaseEntityOutput*)((char *)this + pDataMap->m_fields[k].offset);
					pOutput->AddAction(pszBegin + 1);
				}
			}
		}

		if(pDataMap->baseMap != NULL)
			SetKeyDatamap( pszName, pszValue, pDataMap->baseMap);

		return;
	}

	for(int k = 0; k < pDataMap->m_dataNumFields; k++)
	{
		if((pDataMap->m_fields[k].nFlags & FIELDFLAG_KEY) && !stricmp(pDataMap->m_fields[k].name, pszName))
		{
			if(	pDataMap->m_fields[k].type == VTYPE_STRING ||
				pDataMap->m_fields[k].type == VTYPE_MODELNAME ||
				pDataMap->m_fields[k].type == VTYPE_SOUNDNAME)
			{
				EqString* strPtr = (EqString*)((char *)this + pDataMap->m_fields[k].offset);
				*strPtr = pszValue;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_FLOAT || pDataMap->m_fields[k].type == VTYPE_TIME)
			{
				float* ptr = (float*)((char *)this + pDataMap->m_fields[k].offset);
				*ptr = atof( pszValue );
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_FUNCTION)
			{
				void** function_ptr = (void**)((char *)this + pDataMap->m_fields[k].offset);
				// TODO: find function by reg. name
				//ptr = 

				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_INTEGER)
			{
				int* ptr = (int*)((char *)this + pDataMap->m_fields[k].offset);
				*ptr = atoi( pszValue );
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_BOOLEAN)
			{
				bool* ptr = (bool*)((char *)this + pDataMap->m_fields[k].offset);
				*ptr = atoi( pszValue ) > 0;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_SHORT)
			{
				short* ptr = (short*)((char *)this + pDataMap->m_fields[k].offset);
				*ptr = (short)atoi( pszValue );
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_BYTE)
			{
				int8* ptr = (int8*)((char *)this + pDataMap->m_fields[k].offset);
				*ptr = (int8)atoi( pszValue );
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_VECTOR2D)
			{
				Vector2D* vec = (Vector2D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector2D decode;
				sscanf( pszValue ,"%f %f", &decode.x, &decode.y);

				vec->x = decode.x;
				vec->y = decode.y;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_VECTOR3D)
			{
				Vector3D* vec = (Vector3D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector3D decode;
				sscanf( pszValue ,"%f %f %f", &decode.x, &decode.y, &decode.z);

				vec->x = decode.x;
				vec->y = decode.y;
				vec->z = decode.z;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_VECTOR4D)
			{
				Vector4D* vec = (Vector4D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector4D decode;
				sscanf( pszValue ,"%f %f %f %f", &decode.x, &decode.y, &decode.z, &decode.w);

				vec->x = decode.x;
				vec->y = decode.y;
				vec->z = decode.z;
				vec->w = decode.w;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_ORIGIN ||
				pDataMap->m_fields[k].type == VTYPE_ANGLES)
			{
				Vector3D* vec = (Vector3D *)((char *)this + pDataMap->m_fields[k].offset);
				Vector3D decode;
				sscanf( pszValue ,"%f %f %f", &decode.x, &decode.y, &decode.z);

				vec->x = decode.x;
				vec->y = decode.y;
				vec->z = decode.z;

				//Msg("Angles parsed for %s (%g %g %g)\n", pEnt->GetClassname(), decode.x, decode.y, decode.z);
					
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_ANGLE)
			{
				float* ptr = (float*)((char *)this + pDataMap->m_fields[k].offset);
				*ptr = atof( pszValue );
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_MATRIX2X2)
			{
				Matrix2x2* vec = (Matrix2x2 *)((char *)this + pDataMap->m_fields[k].offset);
				Matrix2x2 decode;
				sscanf(pszValue,"%f %f %f %f", &decode.rows[0].x, &decode.rows[0].y, &decode.rows[1].x, &decode.rows[1].y);

				*vec = decode;
				return;
			}

			if(pDataMap->m_fields[k].type == VTYPE_MATRIX3X3)
			{
				Matrix3x3* vec = (Matrix3x3 *)((char *)this + pDataMap->m_fields[k].offset);

				Matrix3x3 decode;

				sscanf(pszValue,"%f %f %f %f %f %f %f %f %f", 
					&decode.rows[0].x, &decode.rows[0].y, &decode.rows[0].z,
					&decode.rows[1].x, &decode.rows[1].y, &decode.rows[1].z,
					&decode.rows[2].x, &decode.rows[2].y, &decode.rows[2].z);

				Matrix3x3 final;
				final.rows[0] = decode.rows[0]; // z forward
				final.rows[1] = decode.rows[1]; // x
				final.rows[2] = decode.rows[2]; // y 

				*vec = final;
				return;
			}

			else if(pDataMap->m_fields[k].type == VTYPE_MATRIX4X4)
			{
				Matrix4x4* vec = (Matrix4x4 *)((char *)this + pDataMap->m_fields[k].offset);
				Matrix4x4 decode;
				sscanf(pszValue,"%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", 
					&decode.rows[0].x, &decode.rows[0].y, &decode.rows[0].z, &decode.rows[0].w,
					&decode.rows[1].x, &decode.rows[1].y, &decode.rows[1].z, &decode.rows[1].w,
					&decode.rows[2].x, &decode.rows[2].y, &decode.rows[2].z, &decode.rows[2].w);

				*vec = decode;
				return;
			}
		}
	}

	if(pDataMap->baseMap != NULL)
		SetKeyDatamap( pszName, pszValue, pDataMap->baseMap);
}

//-----------------------------------------
// basic inputs
//-----------------------------------------

void BaseEntity::InputAttach(inputdata_t& data)
{
	UpdateTransform();

	// attach caller entity
	SetOwner(data.pCaller);

	if(m_pOwner)
	{
		Matrix4x4 parentToWorld;
		GetParentToWorldTransform(parentToWorld);

		// set my local transform from owner entity and keep being updating
		m_vecOrigin = -parentToWorld.rows[3].xyz();
		Vector3D angles = EulerMatrixXYZ( parentToWorld.getRotationComponent() );
		m_vecAngles = VRAD2DEG( angles );
	}
}

void BaseEntity::InputKill(inputdata_t& data)
{
	SetState(ENTITY_REMOVE);
}

void BaseEntity::InputScriptFunc(inputdata_t& data)
{
	/*
	// execute input on script
	OBJ_BEGINCALL(data.value.GetString()) // value is used as script function name
	{
		call.AddParam(data.pCaller ? gmVariable(data.pCaller->GetScriptUserObject()) : gmVariable(GM_NULL,0));
		call.AddParam(data.pActivator ? gmVariable(data.pActivator->GetScriptUserObject()) : gmVariable(GM_NULL,0));

		call.End();
	}
	OBJ_CALLDONE;
	*/
}