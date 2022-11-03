//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Collision object with shape data
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "eqCollision_Pair.h"

class btCollisionShape;
class btCollisionObject;
struct btTriangleInfoMap;
class CEqBulletIndexedMesh;
class CEqPhysics;
class CEqCollisionObject;
class IEqPhysCallback;
struct studioPhysData_t;
struct collgridcell_t;

const int PHYSICS_COLLISION_LIST_MAX = 8;

enum ECollisionObjectFlags
{
	// disables raycasting and convex sweep casting
	// might be ignored by EQPHYS_FILTER_FLAG_FORCE_RAYCAST flag
	COLLOBJ_NO_RAYCAST				= (1 << 0),

	// enable collision list
	COLLOBJ_COLLISIONLIST			= (1 << 1),

	// disables collision checks by itself, but lets response to be applied to inflictor
	COLLOBJ_DISABLE_COLLISION_CHECK	= (1 << 2),

	// disables collision checks and response to inflictor
	COLLOBJ_DISABLE_RESPONSE		= ((1 << 3) | COLLOBJ_DISABLE_COLLISION_CHECK),

	// is ghost object
	COLLOBJ_ISGHOST					= (1 << 4),

	//---------------
	// special flags

	COLLOBJ_IS_PROCESSING			= (1 << 30),
	COLLOBJ_TRANSFORM_DIRTY			= (1 << 31),
};

class CEqCollisionObject
{
	friend class CEqPhysics;

public:
	CEqCollisionObject();
	virtual ~CEqCollisionObject();

	// objects that will be created
	bool						Initialize(studioPhysData_t* data, int nObject = 0);				///< Convex shape or other
	bool						Initialize(CEqBulletIndexedMesh* mesh, bool internalEdges);			///< Triangle mesh shape TODO: different container
	bool						Initialize(const FVector3D& boxMins, const FVector3D& boxMaxs);		///< bounding box
	bool						Initialize(float radius);											///< sphere
	bool						Initialize(float radius, float height);								///< cylinder

	void						Destroy();															///< destroys the collision model

	virtual void				ClearContacts();

	btCollisionObject*			GetBulletObject() const;											///< returns bullet physics collision object
	btCollisionShape*			GetBulletShape() const;												///< returns bullet physics shape
	CEqBulletIndexedMesh*		GetMesh() const;													///< returns indexed shape

	const Vector3D&				GetShapeCenter() const;

	void						SetUserData(void* ptr);												///< sets user data (usually it's a pointer to game object)
	void*						GetUserData() const;												///< returns user data

	virtual const FVector3D&	GetPosition() const;												///< returns body position
	virtual const Quaternion&	GetOrientation() const;												///< returns body Quaternion orientation

	virtual void				SetPosition(const FVector3D& position);								///< sets new position
	virtual void				SetOrientation(const Quaternion& orient);							///< sets new orientation and updates inertia tensor

	virtual bool				IsDynamic() const {return false;}									///< is dynamic?

	collgridcell_t*				GetCell() const					{return m_cell;}					///< returns collision grid cell
	void						SetCell(collgridcell_t* cell)	{m_cell = cell;}					///< sets collision grid cell

	float						GetFriction() const;
	float						GetRestitution() const;

	void						SetFriction(float value);
	void						SetRestitution(float value);

	void						UpdateBoundingBoxTransform();

	//--------------------

	void						SetContents(int contents);											///< sets this object collision contents accessory
	void						SetCollideMask(int maskContents);									///< sets what collision object contents can collide with this

	int							GetContents() const;
	int							GetCollideMask() const;

	bool						CheckCanCollideWith( CEqCollisionObject* object ) const;					///< just checks possibility of collision, pre-broadphase

	//--------------------

	virtual void				ConstructRenderMatrix( Matrix4x4& outMatrix );						///< constructs render matrix

#ifndef FLOAT_AS_FREAL
	virtual void				ConstructRenderMatrix( FMatrix4x4& outMatrix );						///< constructs render matrix (fixed point)
#endif // FLOAT_AS_FREAL

	void						DebugDraw();

	void						SetDebugName(const char* name);

	//--------------------

	FixedArray<CollisionPairData_t, PHYSICS_COLLISION_LIST_MAX>	m_collisionList;

	BoundingBox					m_aabb;																///< bounding box
	BoundingBox					m_aabb_transformed;													///< transformed bounding box, does not updated in dynamic objects

	IVector4D					m_cellRange;														///< static object cell range for broadphase searching

	IEqPhysCallback*			m_callbacks;

	int							m_surfParam;														///< surface parameters if no CEqBulletIndexedMesh defined
	volatile int				m_flags;															///< collision object flags, ECollisionObjectFlags and EBodyFlags

	float						m_erp;

	//--------------------------------------------------------------------------------
protected:
	void						InitAABB();

#ifdef _DEBUG
	EqString					m_debugName;
#endif // _DEBUG

	Matrix4x4					m_cachedTransform;
	Quaternion					m_orientation;			// floating point Quaternions are ok
	Vector3D					m_center;
	FVector3D					m_position;				// fixed point positions are ideal

	btCollisionObject*			m_collObject;

	CEqBulletIndexedMesh*		m_mesh;
	btTriangleInfoMap*			m_trimap;

	collgridcell_t*				m_cell;

	btCollisionShape*			m_shape;

	btCollisionShape**			m_shapeList;
	int							m_numShapes;			// > 1 indicates it's a compound
	
	void*						m_userData;

	int							m_contents;
	int							m_collMask;

	float						m_restitution;
	float						m_friction;

	bool						m_studioShape;
};
