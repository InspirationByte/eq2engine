//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
struct StudioPhysData;
struct eqPhysGridCell;

const int PHYSICS_COLLISION_LIST_MAX = 8;

enum ECollisionObjectFlags
{
	// disables raycasting and convex sweep casting
	// explicitly ignored by EQPHYS_FILTER_FLAG_FORCE_RAYCAST flag
	COLLOBJ_NO_RAYCAST				= (1 << 0),

	// enable collision list
	COLLOBJ_COLLISIONLIST			= (1 << 1),

	// disables collision checks by itself, but lets response to be applied to inflictor
	COLLOBJ_DISABLE_COLLISION_CHECK	= (1 << 2),

	// disables collision checks and response to inflictor
	COLLOBJ_DISABLE_RESPONSE		= ((1 << 3) | COLLOBJ_DISABLE_COLLISION_CHECK),

	// is ghost object
	COLLOBJ_ISGHOST					= (1 << 4),

	// game flag that marks static objects as moveable (by scripts etc)
	COLLOBJ_MIGHT_MOVE				= (1 << 5),

	//---------------
	// special flags

	COLLOBJ_IS_PROCESSING			= (1 << 29),
	COLLOBJ_TRANSFORM_DIRTY			= (1 << 30),
	COLLOBJ_BOUNDBOX_DIRTY			= (1 << 31)
};

class CEqCollisionObject
{
	friend class CEqPhysics;
	friend class IEqPhysCallback;

public:
	using GetSurfaceParamIdFunc = int(*)(const char*);
	using CollisionPairList = FixedArray<CollisionPairData_t, PHYSICS_COLLISION_LIST_MAX>;

	static GetSurfaceParamIdFunc GetSurfaceParamId;

	CEqCollisionObject();
	virtual ~CEqCollisionObject();

	// objects that will be created
	bool						Initialize(const StudioPhysData* data, int objectIdx = 0);		///< Convex shape or other
	bool						Initialize(CEqBulletIndexedMesh* mesh, bool internalEdges);			///< Triangle mesh shape TODO: different container
	bool						Initialize(const FVector3D& boxMins, const FVector3D& boxMaxs);		///< bounding box
	bool						Initialize(float radius);											///< sphere
	bool						Initialize(float radius, float height);								///< cylinder

	void						Destroy();															///< destroys the collision model

	virtual void				ClearContacts();

	btCollisionObject*			GetBulletObject() const;											///< returns bullet physics collision object
	btCollisionShape*			GetCompoundBulletShape() const { return m_shape; };					///< returns bullet physics shape (compound variant if multiple)
	ArrayCRef<btCollisionShape*>	GetBulletCollisionShapes() const;								///< returns bullet physics shape
	CEqBulletIndexedMesh*		GetMesh() const;													///< returns indexed shape

	const Vector3D&				GetShapeCenter() const;

	void						SetUserData(void* ptr);												///< sets user data (usually it's a pointer to game object)
	void*						GetUserData() const;												///< returns user data

	virtual const FVector3D&	GetPosition() const;												///< returns body position
	virtual const Quaternion&	GetOrientation() const;												///< returns body Quaternion orientation

	virtual void				SetPosition(const FVector3D& position);								///< sets new position
	virtual void				SetOrientation(const Quaternion& orient);							///< sets new orientation and updates inertia tensor

	virtual bool				IsDynamic() const {return false;}									///< is dynamic?

	eqPhysGridCell*				GetCell() const					{return m_cell;}					///< returns collision grid cell
	void						SetCell(eqPhysGridCell* cell)	{m_cell = cell;}					///< sets collision grid cell

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
	void						SetDebugName(const char* name);

	//--------------------
	CollisionPairList		m_collisionList;

	BoundingBox				m_aabb;							///< local shape bounding box
	BoundingBox				m_aabb_transformed;				///< transformed bounding box, does not updated in dynamic objects
	IAARectangle			m_cellRange{ 0, 0, 0, 0 };		///< static object cell range for broadphase searching

	int						m_surfParam{ 0 };					///< surface parameters if no CEqBulletIndexedMesh defined
	int						m_flags{ COLLOBJ_TRANSFORM_DIRTY };	///< collision object flags, ECollisionObjectFlags and EBodyFlags

	float					m_erp{ 0.0f };

	//--------------------------------------------------------------------------------
protected:
	void					InitAABB();

#ifdef _DEBUG
	EqString				m_debugName;
#endif // _DEBUG

	Matrix4x4				m_cachedTransform{ identity4 };
	Quaternion				m_orientation{ qidentity };		// floating point Quaternions are ok
	Vector3D				m_center{ vec3_zero };
	FVector3D				m_position{ 0.0f };				// fixed point positions are ideal
	
	IEqPhysCallback*		m_callbacks{ nullptr };
	void*					m_userData{ nullptr };

	btCollisionObject*		m_collObject{ nullptr };
	CEqBulletIndexedMesh*	m_mesh{ nullptr };
	btTriangleInfoMap*		m_trimap{ nullptr };
	eqPhysGridCell*			m_cell{ nullptr };

	btCollisionShape*		m_shape{ nullptr };
	btCollisionShape**		m_shapeList{ nullptr };
	int						m_numShapes{ 0 };			// > 1 indicates it's a compound

	int						m_contents{ (int)0xffffffff };
	int						m_collMask{ (int)0xffffffff };

	float					m_restitution{ 0.1f };
	float					m_friction{ 0.1f };

	bool					m_studioShape{ false };
};
