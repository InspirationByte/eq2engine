//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Collision object with shape data
//////////////////////////////////////////////////////////////////////////////////

#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "egf/model.h"
#include "eqCollision_Object.h"

#include "physics/BulletConvert.h"
#include "eqBulletIndexedMesh.h"

#include "materialsystem1/IMaterialSystem.h"

using namespace EqBulletUtils;

DECLARE_CVAR(ph_margin, "0.0001", nullptr, CV_CHEAT | CV_UNREGISTERED);

static constexpr const float EQPHYSICS_AABB_EXPAND = 0.15f;

CEqCollisionObject::GetSurfaceParamIdFunc CEqCollisionObject::GetSurfaceParamId = nullptr;

CEqCollisionObject::CEqCollisionObject()
{
}

CEqCollisionObject::~CEqCollisionObject()
{
	Destroy();
}

void CEqCollisionObject::Destroy()
{
	SAFE_DELETE(m_collObject);

	if (!m_studioShape)
	{
		SAFE_DELETE(m_shape);
	}

	m_studioShape = false;

	SAFE_DELETE(m_trimap);

	m_shape = nullptr;
	m_mesh = nullptr;
	m_collObject = nullptr;
	m_trimap = nullptr;
}

void CEqCollisionObject::ClearContacts()
{
	m_collisionList.clear(false);
}

void CEqCollisionObject::InitAABB()
{
	if(!m_shape)
		return;

	// get shape mins/maxs
	btTransform trans;
	trans.setIdentity();

	btVector3 mins,maxs;
	m_shape->getAabb(trans, mins, maxs);

	ConvertBulletToDKVectors(m_aabb.minPoint, mins);
	ConvertBulletToDKVectors(m_aabb.maxPoint, maxs);

	m_aabb_transformed = m_aabb;
}

// objects that will be created
bool CEqCollisionObject::Initialize(const studioPhysData_t* data, int objectIdx)
{
	ASSERT(!m_shape);

	// TODO: make it
	ASSERT_MSG(objectIdx >= 0 && (objectIdx < data->numObjects), "CEqCollisionObject::Initializet - objectIdx is out of numObjects");

	const studioPhysObject_t& physObject = data->objects[objectIdx];

	// as this an actual array of shapes, handle it as array of shapes xD
	m_numShapes = physObject.object.numShapes;
	m_shapeList = (btCollisionShape**)physObject.shapeCache;

	ASSERT_MSG(GetSurfaceParamId != nullptr, "Must set up CEqCollisionObject::GetSurfaceParamId callback for your physics engine");
	m_surfParam = GetSurfaceParamId(physObject.object.surfaceprops);

	// setup default shape
	if (m_numShapes > 1)
	{
		btTransform ident;
		ident.setIdentity();

		btCollisionShape** shapes = (btCollisionShape**)physObject.shapeCache;

		btCompoundShape* compound = new btCompoundShape(false, m_numShapes);
		for (int i = 0; i < m_numShapes; i++)
			compound->addChildShape(ident, m_shapeList[i]);

		m_shape = compound;
		m_studioShape = false;
	}
	else
	{
		m_shape = m_shapeList[0];
		m_studioShape = true; // do not delete!
	}
	
	ASSERT_MSG(m_shape, "No valid shape!");

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_collObject->setUserPointer(this);

	return true;
}

bool CEqCollisionObject::Initialize( CEqBulletIndexedMesh* mesh, bool internalEdges )
{
	ASSERT(!m_shape);

	m_mesh = mesh;

	m_numShapes = 1;
	m_shapeList = nullptr;

	btBvhTriangleMeshShape* meshShape = new btBvhTriangleMeshShape(m_mesh, true, true);

	if (internalEdges)
	{
		// WARNING: this is slow!
		m_trimap = PPNew btTriangleInfoMap();
		btGenerateInternalEdgeInfo(meshShape, m_trimap);
	}

	m_shape = meshShape;
	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_collObject->setUserPointer(this);

	m_studioShape = false;

	return true;
}

bool CEqCollisionObject::Initialize(const FVector3D& boxMins, const FVector3D& boxMaxs)
{
	ASSERT(!m_shape);

	btVector3 vecHalfExtents;
	Vector3D ext = (boxMaxs - boxMins) * 0.5f;
	
	ConvertDKToBulletVectors(vecHalfExtents, ext);

	m_center = (boxMins+boxMaxs)*0.5f;

	btBoxShape* box = new btBoxShape( vecHalfExtents );
	box->initializePolyhedralFeatures();

	m_numShapes = 1;
	m_shapeList = nullptr;

	m_shape = box;
	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject->setUserPointer(this);

	m_studioShape = false;

	return true;
}

bool CEqCollisionObject::Initialize(float radius)
{
	ASSERT(!m_shape);

	m_numShapes = 1;
	m_shapeList = nullptr;


	m_shape = new btSphereShape(radius);
	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject->setUserPointer(this);

	m_studioShape = false;

	return true;
}

bool CEqCollisionObject::Initialize(float radius, float height)
{
	ASSERT(!m_shape);

	m_numShapes = 1;
	m_shapeList = nullptr;

	m_shape = new btCylinderShape(btVector3(radius, height, radius));
	m_collObject = new btCollisionObject();
	m_collObject->setCollisionShape(m_shape);

	m_shape->setMargin(ph_margin.GetFloat());

	InitAABB();

	m_collObject->setUserPointer(this);

	m_studioShape = false;

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
	m_flags |= COLLOBJ_TRANSFORM_DIRTY | COLLOBJ_BOUNDBOX_DIRTY;

	UpdateBoundingBoxTransform();
}

void CEqCollisionObject::SetOrientation(const Quaternion& orient)
{
	m_orientation = orient;
	m_flags |= COLLOBJ_TRANSFORM_DIRTY | COLLOBJ_BOUNDBOX_DIRTY;

	UpdateBoundingBoxTransform();
}

void CEqCollisionObject::UpdateBoundingBoxTransform()
{
	if ((m_flags & COLLOBJ_BOUNDBOX_DIRTY) == 0)
		return;
	m_flags &= ~COLLOBJ_BOUNDBOX_DIRTY;

	BoundingBox srcBox = m_aabb;
	BoundingBox finalBox;

	for(int i = 0; i < BoundingBox::VertexCount; i++)
		finalBox.AddVertex(rotateVector(srcBox.GetVertex(i), m_orientation));

	const Vector3D offset = m_position;
	finalBox.Expand(EQPHYSICS_AABB_EXPAND);
	finalBox.minPoint += offset;
	finalBox.maxPoint += offset;

	m_aabb_transformed = finalBox;
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
	if(m_flags & COLLOBJ_TRANSFORM_DIRTY)
	{
		Matrix4x4 transform = Matrix4x4(m_orientation);
		transform.setTranslationTransposed(Vector3D(m_position));
		m_cachedTransform = transform;
		m_flags &= ~COLLOBJ_TRANSFORM_DIRTY;
	}

	outMatrix = m_cachedTransform;
}

void CEqCollisionObject::SetDebugName(const char* name)
{
#ifdef _DEBUG
	m_debugName = name;
#endif // _DEBUG
}

