//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera for Engine
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "model.h"
#include "Effects.h"
#include "utils/geomtools.h"
#include "BaseAnimating.h"
#include "ragdoll.h"

extern IModelCache* g_pModelCache;
ConVar r_drawStaticProps("r_drawStaticProps", "1", "Draw static models (props)\n");
ConVar r_drawPhysicsProps("r_drawPhysicsProps", "1", "Draw physics props\n");

ConVar r_testSpeed("r_testSpeed", "5", "Test walk rot speed\n");

class CDynamicProp : public BaseAnimating
{
	DEFINE_CLASS_BASE(BaseAnimating);

public:
	CDynamicProp()
	{
		m_fPoseParam = 0.0f;
	}

	void Precache()
	{
		PrecacheStudioModel( m_pszModelName.GetData() );
		BaseClass::Precache();
	}

	void Spawn()
	{
		SetModel(m_pszModelName.GetData());

		BaseClass::Spawn();

		PhysicsCreateObjects();

		SetActivity(ACT_RUN);
	}

	void PhysicsCreateObjects()
	{
		if(m_pPhysicsObject)
			return;

		IPhysicsObject* pObject = PhysicsInitStatic();

		if(pObject)
		{
			pObject->SetCollisionMask(COLLIDE_OBJECT);
			pObject->SetContents(COLLISION_GROUP_OBJECTS);

			pObject->SetPosition(GetAbsOrigin());
			pObject->SetAngles(GetAbsAngles());
		}
	}

	void OnPreRender()
	{
		m_fPoseParam += gpGlobals->frametime * r_testSpeed.GetFloat();

		Vector3D fw;
		AngleVectors(Vector3D(0,GetAbsAngles().y-m_fPoseParam, 0), &fw);

		debugoverlay->Line3D(GetAbsOrigin(), GetAbsOrigin()+fw*200.0f, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,0));

		static int nPoseParam = FindPoseController("move_yaw");
		SetPoseControllerValue(nPoseParam, m_fPoseParam);

		if(m_fPoseParam > 180.0f)
			m_fPoseParam = -180.0f;

		if(m_fPoseParam < -180.0f)
			m_fPoseParam = 180.0f;

		UpdateBones();

		//if(GetCurrentRemainingAnimationDuration() < 0.01f)
		//	ResetAnimationTime();

		g_modelList->AddRenderable(this);
	}

	// min bbox dimensions
	Vector3D GetBBoxMins()
	{
		return m_pModel->GetBBoxMins();
	}

	// max bbox dimensions
	Vector3D GetBBoxMaxs()
	{
		return m_pModel->GetBBoxMaxs();
	}

protected:

	float		m_fPoseParam;
};

DECLARE_ENTITYFACTORY(prop_dynamic,CDynamicProp) // static prop


class CStaticPropModel : public BaseEntity
{
	DEFINE_CLASS_BASE(BaseEntity);

public:
	CStaticPropModel()
	{
		m_pRooms[0] = -1;
		m_pRooms[1] = -1;

		m_bCastShadows = true;
	}

	void Precache()
	{
		PrecacheStudioModel( m_pszModelName.GetData() );
		BaseClass::Precache();
	}

	void Spawn()
	{
		SetModel(m_pszModelName.GetData());

		PhysicsCreateObjects();

		BaseClass::Spawn();
	}

	void PhysicsCreateObjects()
	{
		if(m_pPhysicsObject || !m_pModel)
			return;

		IPhysicsObject* pObject = PhysicsInitStatic();

		if(pObject)
		{
			pObject->SetCollisionMask(COLLIDE_OBJECT);
			pObject->SetContents(COLLISION_GROUP_OBJECTS);

			pObject->SetPosition(GetAbsOrigin());
			pObject->SetAngles(GetAbsAngles());
		}
	}

	void OnPreRender()
	{
		g_modelList->AddRenderable(this);
	}

	void Render(int nViewRenderFlags)
	{
		if(!r_drawStaticProps.GetBool())
			return;

		if(!m_bCastShadows && viewrenderer->GetDrawMode() == VDM_SHADOW)
			return;

		BaseClass::Render( nViewRenderFlags );
	}
protected:
	bool		m_bCastShadows;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CStaticPropModel)

	DEFINE_KEYFIELD( m_bCastShadows,		"castshadows", VTYPE_BOOLEAN),

END_DATAMAP()

DECLARE_ENTITYFACTORY(prop_static,CStaticPropModel) // static prop

class CPhysBox : public BaseAnimating
{
	DEFINE_CLASS_BASE(BaseAnimating);

public:
	CPhysBox()
	{
		m_bStartAsleep = false;
	}

	void Precache()
	{
		PrecacheStudioModel( m_pszModelName.GetData() );
		BaseClass::Precache();
	}

	void Spawn()
	{
		SetModel(m_pszModelName.GetData());

		BaseClass::Spawn();

		PhysicsCreateObjects();
	}

	void PhysicsCreateObjects()
	{
		if(m_pPhysicsObject)
			return;

		IPhysicsObject* pObject = PhysicsInitNormal(0, m_bStartAsleep);

		if(pObject)
		{
			pObject->SetCollisionMask(COLLIDE_OBJECT);
			pObject->SetContents(COLLISION_GROUP_OBJECTS);

			pObject->SetPosition(GetAbsOrigin());
			pObject->SetAngles(GetAbsAngles());
		}
		else
		{
			MsgError("There is no physics model for %s\n", m_pszModelName.GetData());
			SetState(ENTITY_REMOVE);
		}
	}

	void FirstUpdate()
	{
		if(!m_bStartAsleep && PhysicsGetObject())
			PhysicsGetObject()->WakeUp();
	}

	void OnPreRender()
	{
		UpdateBones();

		g_modelList->AddRenderable(this);
	}

	void Update(float dt)
	{
		if(!PhysicsGetObject())
		{
			BaseClass::Update(dt);
			return;
		}

		SetAbsOrigin(PhysicsGetObject()->GetPosition());
		SetAbsAngles(PhysicsGetObject()->GetAngles());

		ObjectSoundEvents( PhysicsGetObject(), this );

		BaseClass::Update(dt);
	}

private:
	bool m_bStartAsleep;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CPhysBox)
	DEFINE_KEYFIELD( m_bStartAsleep,	"sleep", VTYPE_BOOLEAN),
END_DATAMAP()

DECLARE_ENTITYFACTORY(func_physbox,CPhysBox) // and func_static with one definition
DECLARE_ENTITYFACTORY(prop_physics,CPhysBox) // and func_static with one definition

#define RAGDOLL_MODEL "models/characters/test.egf"
//#define RAGDOLL_MODEL "models/characters/devushka.egf"

class CTempRagdoll : public BaseAnimating
{
	DEFINE_CLASS_BASE(BaseAnimating);

public:
	CTempRagdoll()
	{
		m_pRagdoll = NULL;
	}

	void Precache()
	{
		g_pModelCache->PrecacheModel(RAGDOLL_MODEL);
		BaseClass::Precache();
	}

	void Spawn()
	{
		SetModel(RAGDOLL_MODEL);

		BaseClass::Spawn();

		// calculate absolute transformation of bones
		//SetActivity(ACT_RUN);

		m_fRemainingTransitionTime = 0.0f;

		AdvanceFrame(0);
		UpdateBones();

		m_pRagdoll = CreateRagdoll(m_pModel);

		Matrix4x4 object_transform = ComputeWorldMatrix(vec3_zero ,m_vecAbsAngles, Vector3D(1));

		for(int i = 0; i < m_numBones; i++)
			m_BoneMatrixList[i] = m_BoneMatrixList[i] * object_transform;

		m_pRagdoll->SetBoneTransform(m_BoneMatrixList, identity4());

		//m_pRagdoll->m_pJoints[0]->GetPhysicsObjectA()->SetLinearFactor(vec3_zero);
		//m_pRagdoll->m_pJoints[0]->GetPhysicsObjectA()->SetAngularFactor(vec3_zero);
		
		m_pRagdoll->Translate(m_vecAbsOrigin);

		//m_pRagdoll->Wake();
	}

	void OnRemove()
	{
		DestroyRagdoll(m_pRagdoll);

		BaseClass::OnRemove();
	}

	void OnPreRender()
	{
		g_modelList->AddRenderable(this);
	}

	void UpdateTransform()
	{
		if(!m_pRagdoll)
			return;

		m_pRagdoll->GetVisualBonesTransforms( m_BoneMatrixList );

		Vector3D ragdoll_origin = m_pRagdoll->GetPosition();

		m_pRagdoll->GetBoundingBox(m_vMins, m_vMaxs);

		m_vMins -= ragdoll_origin;
		m_vMaxs -= ragdoll_origin;

		m_vecAbsOrigin = ragdoll_origin;
		m_vecAbsAngles = vec3_zero;

		BaseClass::UpdateTransform();
	}

	void OnPostRender()
	{
		for(int i = 0; i < m_pRagdoll->m_numParts; i++)
		{
			if(m_pRagdoll->m_pParts[i])
				ObjectSoundEvents(m_pRagdoll->m_pParts[i], this);
		}
	}

	void Update(float dt)
	{
		BaseClass::Update(dt);
	}

	// min bbox dimensions
	Vector3D GetBBoxMins()
	{
		return m_vMins;
	}

	// max bbox dimensions
	Vector3D GetBBoxMaxs()
	{
		return m_vMaxs;
	}

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform()
	{
		return ComputeWorldMatrix(m_vecAbsOrigin, m_vecAbsAngles, Vector3D(1));
	}

private:

	ragdoll_t*			m_pRagdoll;
	Vector3D			m_vMins;
	Vector3D			m_vMaxs;
	
};

DECLARE_ENTITYFACTORY(ragdoll,CTempRagdoll) // and func_static with one definition
