//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Collision object with shape data
//////////////////////////////////////////////////////////////////////////////////


#include "eqCollision_Object.h"

#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"

#include "eqBulletIndexedMesh.h"

#include "../shared_engine/physics/BulletConvert.h"

#include "materialsystem/IMaterialSystem.h"

#include "ConVar.h"

using namespace EqBulletUtils;

ConVar ph_margin("ph_margin", "0.0001", "no desc", CV_CHEAT);

#define AABB_GROWVALUE	 (0.15f)

CEqCollisionObject::CEqCollisionObject()
{
	m_collObject = NULL;
	m_shape = NULL;
	m_mesh = NULL;
	m_userData = NULL;
	m_center = vec3_zero;
	m_surfParam = 0;
	m_hasStudioShape = false;
	m_trimap = NULL;
	m_cell = NULL;
	m_erp = 0.0f;

	m_restitution = 0.1f;
	m_friction = 0.1f;

	m_position = FVector3D(0);
	m_orientation = identity();

	m_cellRange = IVector4D(0,0,0,0);

	m_contents = COLLISION_MASK_ALL;
	m_collMask = COLLISION_MASK_ALL;

	m_flags = 0;

	m_cachedTransformDirty = false;
	m_cachedTransform = identity4();
}

CEqCollisionObject::~CEqCollisionObject()
{
	Destroy();
}

void CEqCollisionObject::Destroy()
{
	delete m_collObject;

	if(!m_hasStudioShape)
		delete m_shape;

	delete m_trimap;

	m_shape = NULL;
	m_mesh = NULL;
	m_collObject = NULL;
	m_trimap = NULL;
}

void CEqCollisionObject::InitAABB()
{
	if(!m_shape)
		return;

	// get shape mins/maxs
	btTransform trans;
	trans.setIdentity();

	btVector3 mins,maxs;
	m_shape->calculateTemporalAabb(trans, btVector3(0,0,0), btVector3(0,0,0), 0.0f, mins, maxs);
	//m_shape->getAabb(trans, mins, maxs);

	m_aabb.minPoint = ConvertBulletToDKVectors(mins);
	m_aabb.maxPoint = ConvertBulletToDKVectors(maxs);

	m_aabb_transformed = m_aabb;
}

// objects that will be created
bool CEqCollisionObject::Initialize( studioPhysData_t* data, int nObject )
{
	// TODO: make it
	ASSERTMSG((nObject < data->numObjects), "DkPhysics::CreateObject - nObject is out of numObjects");

	// first determine shapes
	if(data->objects[nObject].object.numShapes > 1)
	{
		btCompoundShape* pCompoundShape = new btCompoundShape;
		m_shape = pCompoundShape;

		for(int i = 0; i < data->objects[nObject].object.numShapes; i++)
		{
			btTransform ident;
			ident.setIdentity();

			btCollisionShape* shape = (btCollisionShape*)data->objects[nObject].shapeCache[i];

			pCompoundShape->addChildShape(ident, shape);
		}
		
		m_hasStudioShape = false;
	}
	else
	{
		// not a compound object, skipping
		m_shape = (btCollisionShape*)data->objects[nObject].shapeCache[0];
		m_hasStudioShape = true; // using a studio shape flag
	}

	ASSERTMSG(m_shape, "No valid shape!");

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_collObject->setUserPointer(this);

	return true;
}

bool CEqCollisionObject::Initialize( CEqBulletIndexedMesh* mesh )
{
	m_mesh = mesh;

	m_shape = new btBvhTriangleMeshShape(m_mesh, false);
	m_shape->setMargin(ph_margin.GetFloat());

	m_trimap = new btTriangleInfoMap();
	btGenerateInternalEdgeInfo((btBvhTriangleMeshShape*)m_shape, m_trimap);

	InitAABB();

	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_collObject->setUserPointer(this);

	m_hasStudioShape = false;

	return true;
}

bool CEqCollisionObject::Initialize(const FVector3D& boxMins, const FVector3D& boxMaxs)
{
	btVector3 vecHalfExtents(EqBulletUtils::ConvertDKToBulletVectors(boxMaxs-boxMins)*0.5f);

	m_center = (boxMins+boxMaxs)*0.5f;

	btBoxShape* box = new btBoxShape( vecHalfExtents );
	box->initializePolyhedralFeatures();

	m_shape = box;
	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject->setUserPointer(this);

	m_hasStudioShape = false;

	return true;
}

bool CEqCollisionObject::Initialize(float radius)
{
	m_shape = new btSphereShape(radius);
	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject->setUserPointer(this);

	m_hasStudioShape = false;

	return true;
}

btCollisionObject* CEqCollisionObject::GetBulletObject() const
{
	return m_collObject;
}

btCollisionShape* CEqCollisionObject::GetBulletShape() const
{
	return m_shape;
}

CEqBulletIndexedMesh* CEqCollisionObject::GetMesh() const
{
	return m_mesh;
}

const Vector3D& CEqCollisionObject::GetShapeCenter() const
{
	return m_center;
}

void CEqCollisionObject::SetUserData(void* ptr)
{
	m_userData = ptr;
}

void* CEqCollisionObject::GetUserData() const
{
	return m_userData;
}

//--------------------

const FVector3D& CEqCollisionObject::GetPosition() const
{
	return m_position;
}

const Quaternion& CEqCollisionObject::GetOrientation() const
{
	return m_orientation;
}

//--------------------

void CEqCollisionObject::SetPosition(const FVector3D& position)
{
	m_position = position;
	m_cachedTransformDirty = true;

	UpdateBoundingBoxTransform();
}

void CEqCollisionObject::SetOrientation(const Quaternion& orient)
{
	m_orientation = orient;
	m_cachedTransformDirty = true;

	UpdateBoundingBoxTransform();
}

void CEqCollisionObject::UpdateBoundingBoxTransform()
{
	Matrix4x4 mat;
	ConstructRenderMatrix(mat);

	m_aabb_transformed.Reset();

	for(int i = 0; i < 8; i++)
		m_aabb_transformed.AddVertex((mat*Vector4D(m_aabb.GetVertex(i), 1.0f)).xyz());

	m_aabb_transformed.maxPoint += AABB_GROWVALUE;
	m_aabb_transformed.minPoint -= AABB_GROWVALUE;
}

//------------------------------

float CEqCollisionObject::GetFriction() const
{
	return m_friction;
}

float CEqCollisionObject::GetRestitution() const
{
	return m_restitution;
}

void CEqCollisionObject::SetFriction(float value)
{
	m_friction = value;
}

void CEqCollisionObject::SetRestitution(float value)
{
	m_restitution = value;
}

//-----------------------------

void CEqCollisionObject::SetContents(int contents)
{
	m_contents = contents;
}

void CEqCollisionObject::SetCollideMask(int maskContents)
{
	m_collMask = maskContents;
}

int	CEqCollisionObject::GetContents() const
{
	return m_contents;
}

int CEqCollisionObject::GetCollideMask() const
{
	return m_collMask;
}

// logical check, pre-broadphase
bool CEqCollisionObject::CheckCanCollideWith( CEqCollisionObject* object ) const
{
	if((GetContents() & object->GetCollideMask()) || (GetCollideMask() & object->GetContents()))
	{
		return true;
	}

	return false;
}

//-----------------------------

void CEqCollisionObject::ConstructRenderMatrix( Matrix4x4& outMatrix )
{
	if(m_cachedTransformDirty)
	{
		Matrix4x4 rotation = Matrix4x4(m_orientation);
		m_cachedTransform = translate(Vector3D(m_position)) * rotation;
		m_cachedTransformDirty = false;

		UpdateBoundingBoxTransform();
	}

	outMatrix = m_cachedTransform;
}

#ifndef FLOAT_AS_FREAL
void CEqCollisionObject::ConstructRenderMatrix( FMatrix4x4& outMatrix )
{
	if( m_orientation.isNan() )
		m_orientation = identity();

	outMatrix = FMatrix4x4(m_orientation);
	outMatrix = translate(m_position) * outMatrix;
}

void CEqCollisionObject::DebugDraw()
{
	if(m_hasStudioShape)
	{
		Matrix4x4 m;
		ConstructRenderMatrix(m);

		materials->SetMatrix(MATRIXMODE_WORLD,m);
	}
}

void CEqCollisionObject::SetDebugName(const char* name)
{
#ifdef _DEBUG
	m_debugName = name;
#endif // _DEBUG
}

#endif // FLOAT_AS_FREAL
