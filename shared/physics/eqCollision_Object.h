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
struct eqPhysBroadphaseUnit;
class btBroadphaseInterface;

class CEqBulletIndexedMesh;
class IEqPhysCallback;
struct StudioPhysData;
struct StudioPhyObjData;

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
	// this will also create broadphase grid cells if transform has changed
	COLLOBJ_MIGHT_MOVE				= (1 << 5),

	//---------------
	// special flags

	COLLOBJ_IS_PROCESSING			= (1 << 28),
	COLLOBJ_BOUNDBOX_DIRTY			= (1 << 29),
	COLLOBJ_BROADPHASE_DIRTY		= (1 << 30),
};

class CEqCollisionObject
{
	friend class CEqPhysicsWorld;
	friend class IEqPhysCallback;

public:
	using GetSurfaceParamIdFunc = int(*)(const char*);
	using CollisionPairList = FixedArray<eqCollisionPairData, PHYSICS_COLLISION_LIST_MAX>;
	using CollisionPairListRef = ArrayCRef<eqCollisionPairData>;
	using CollisionShapeCRefs = ArrayCRef<btCollisionShape*>;

	static GetSurfaceParamIdFunc GetSurfaceParamId;

	CEqCollisionObject() = default;
	virtual ~CEqCollisionObject();

	// objects that will be created
	bool					Initialize(const StudioPhysData& physData, int objIdx);				///< Studio data with physics object id
	bool					Initialize(const StudioPhyObjData& physObject);						///< Studio physics object
	bool					Initialize(CEqBulletIndexedMesh* mesh, bool internalEdges);			///< Triangle mesh shape TODO: different container
	bool					Initialize(const FVector3D& boxMins, const FVector3D& boxMaxs);		///< bounding box
	bool					Initialize(float radius);											///< sphere
	bool					Initialize(float radius, float height);								///< cylinder

	void					Destroy();															///< destroys the collision model

	btCollisionShape*		GetCompoundBulletShape() const { return m_shape; }					///< returns bullet physics shape (compound variant if multiple)
	CollisionShapeCRefs		GetBulletCollisionShapes() const;								///< returns bullet physics shape
	CEqBulletIndexedMesh*	GetMesh() const { return m_mesh; }									///< returns indexed shape

	const Vector3D&			GetShapeCenter() const { return m_center; }

	void					SetUserData(void* ptr) { m_userData = ptr; }						///< sets user data (usually it's a pointer to game object)
	void*					GetUserData() const { return m_userData; }							///< returns user data

	const FVector3D&		GetPosition() const { return m_position; }							///< returns body position
	const Quaternion&		GetOrientation() const { return m_orientation; }					///< returns body Quaternion orientation
	const Transform3D		GetTransform() const;

	virtual void			SetPosition(const FVector3D& position);								///< sets new position
	virtual void			SetOrientation(const Quaternion& orient);							///< sets new orientation and updates inertia tensor
	virtual void			SetTransform(const Transform3D& trs);

	virtual bool			IsDynamic() const { return false; }									///< is dynamic?

	float					GetFriction() const { return m_friction; }
	float					GetRestitution() const { return m_restitution; }
	float					GetErp() const { return m_erp; }

	void					SetFriction(float value) { m_friction = value; }
	void					SetRestitution(float value) { m_restitution = value; }
	void					SetErp(float value) { m_erp = value; }
	
	int						GetFlags() const { return m_flags; }
	void					SetFlags(int value) { m_flags = value; }
	void					AddFlags(int value) { m_flags |= value; }
	void					RemoveFlags(int value) { m_flags &= ~value; }

	int						GetSurfParamId() const { return m_surfParamId; }

	//--------------------

	void					SetContents(int contents) { m_contents = contents; }				///< sets this object collision contents accessory
	void					SetCollideMask(int maskContents) { m_collMask = maskContents; }		///< sets what collision object contents can collide with this

	int						GetContents() const { return m_contents; }
	int						GetCollideMask() const { return m_collMask; }

	//--------------------

	void					UpdateBoundingBoxTransform();

	const BoundingBox&		GetLocalAABB() const { return m_localAABB; }
	const BoundingBox&		GetWorldAABB() const { return m_worldAABB; }

	CollisionPairListRef	GetCollisionList() const { return m_collisionList; }

	void					SetDebugName(const char* name);

	//--------------------------------------------------------------------------------
protected:
	virtual void			ClearContacts();
	void					InitAABB();

	CollisionPairList		m_collisionList;

#ifdef _DEBUG
	EqString				m_debugName;
#endif // _DEBUG

	Quaternion				m_orientation{ qidentity };		// floating point Quaternions are ok
	Vector3D				m_center{ vec3_zero };
	FVector3D				m_position{ 0.0f };				// fixed point positions are ideal

	BoundingBox				m_localAABB;			///< local shape bounding box
	BoundingBox				m_worldAABB;			///< transformed bounding box, does not updated in dynamic objects

	IEqPhysCallback*		m_callbacks{ nullptr };
	void*					m_userData{ nullptr };

	btCollisionObject*		m_collObject{ nullptr };
	eqPhysBroadphaseUnit*	m_broadphaseUnit{ nullptr };

	CEqBulletIndexedMesh*	m_mesh{ nullptr };
	btTriangleInfoMap*		m_trimap{ nullptr };

	btCollisionShape*		m_shape{ nullptr };
	btCollisionShape**		m_shapeList{ nullptr };
	int						m_numShapes{ 0 };			// > 1 indicates it's a compound
	int						m_surfParamId{ 0 };		///< surface parameters if no CEqBulletIndexedMesh defined

	int						m_flags{ 0 };			///< collision object flags, ECollisionObjectFlags and EBodyFlags

	int						m_contents{ (int)COM_UINT_MAX };
	int						m_collMask{ (int)COM_UINT_MAX };

	float					m_restitution{ 0.1f };
	float					m_friction{ 0.1f };
	float					m_erp{ 0.0f };

	enum EShapeOwning : uint8
	{
		OWNS_SHAPE = (1 << 0),
		OWNS_SHAPE_LIST = (1 << 1),
	};

	uint8					m_shapeOwning{ 0 };
};
