//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: static object
//////////////////////////////////////////////////////////////////////////////////

#include "object_tree.h"
#include "materialsystem/IMaterialSystem.h"
#include "world.h"

//-----------------------------------------------------------------------

CObject_Tree::CObject_Tree( kvkeybase_t* kvdata )
{
	m_keyValues = kvdata;
	m_pPhysicsObject = NULL;
	m_blbList = NULL;
}

CObject_Tree::~CObject_Tree()
{

}

void CObject_Tree::OnRemove()
{
	CGameObject::OnRemove();

	if(m_pPhysicsObject)
	{
		m_pPhysicsObject->SetUserData(NULL);
		g_pPhysics->m_physics.DestroyStaticObject(m_pPhysicsObject);
	}
	m_pPhysicsObject = NULL;
}

void CObject_Tree::Spawn()
{
	SetModel( KV_GetValueString(m_keyValues->FindKeyBase("model"), 0, "models/error.egf") );

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_objectInstFormat, g_pGameWorld->m_objectInstBuffer );

		m_pModel->SetInstancer( instancer );
	}

	m_blbList = g_pGameWorld->FindBillboardList(KV_GetValueString(m_keyValues->FindKeyBase("billboardListName")));

	m_smashSound = KV_GetValueString(m_keyValues->FindKeyBase("smashsound"), 0, "");

	ses->PrecacheSound(m_smashSound.c_str());
	
	m_pPhysicsObject = new CEqCollisionObject();

	if( m_pPhysicsObject->Initialize(&m_pModel->GetHWData()->physModel, 0) )
	{
		physobject_t* obj = &m_pModel->GetHWData()->physModel.objects[0].object;

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

	//m_keyValues.Cleanup();

	// baseclass spawn
	CGameObject::Spawn();
}

void CObject_Tree::SetOrigin(const Vector3D& origin)
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->SetPosition( origin );

	m_vecOrigin = origin;
}

void CObject_Tree::SetAngles(const Vector3D& angles)
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CObject_Tree::SetVelocity(const Vector3D& vel)
{

}

extern ConVar r_enableObjectsInstancing;

void CObject_Tree::Draw( int nRenderFlags )
{
	//if(!g_pGameWorld->m_frustum.IsSphereInside(GetOrigin(), length(m_pModel->GetBBoxMaxs())))
	//	return;

	m_pPhysicsObject->ConstructRenderMatrix(m_worldMatrix);

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
		CGameObject::Draw( nRenderFlags );

	// draw
	// TODO: lights
	if(m_blbList)
	{
		materials->SetMatrix( MATRIXMODE_WORLD, m_worldMatrix);
		m_blbList->DrawBillboards();
	}
}

void CObject_Tree::Simulate(float fDt)
{

}

void CObject_Tree::OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy)
{
	// TODO: drop leaves
}