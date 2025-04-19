//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqStudioGeom;
class IPhysicsObject;
class IPhysicsJoint;
struct StudioJoint;
enum EPhysicsActivationState : int;

class CPhysRagdollData
{
public:
	~CPhysRagdollData();
	CPhysRagdollData(CEqStudioGeom* pModel);

	// get bones transformation for rendering (NOTE: before this operation, reset input bone transform to identity)
	void				GetVisualBonesTransforms(Matrix4x4 *bones) const;

	BoundingBox			GetBoundingBox() const;
	Vector3D			GetPosition() const;

	// sets bone tranformations (useful for animated death, etc)
	// you can setup from here a global transform by multipling all matrices on model transform
	void				SetBoneTransform(Matrix4x4 *bones, const Matrix4x4& translation);

	// wakes ragdoll
	void				Wake();
	void				Freeze();

	void				SetContents(int contents);
	void				SetCollisionMask(int mask);

	void				SetCollisionResponseEnabled(bool enable);
	void				SetActivationState(EPhysicsActivationState state);

	void				RefreshRagdollVisuals();
	void				Translate(const Vector3D &move);
	void				ResetVelocities();

	const Matrix4x4&	GetJointTransformA(int idx) const;
	int					GetGeomIdx(int jointIdx) const { return m_jointToGeomIds[jointIdx]; }
	int					GetJointIdx(int geomIdx) const { return m_geomToJointIds[geomIdx]; }

protected:
	// finds far parent bone in ragdoll
	int					ComputeAndGetFarParentOf(int bone);

	static constexpr int MAX_RAGDOLL_PARTS = 32;
	static constexpr int MAX_RAGDOLL_JOINT_IDS = 128;

	ArrayCRef<StudioJoint>	m_studioJoints{ nullptr };
	IPhysicsObject*		m_partObjs[MAX_RAGDOLL_PARTS]{ nullptr };
	IPhysicsJoint*		m_physJoints[MAX_RAGDOLL_PARTS]{ nullptr };

	int					m_jointToGeomIds[MAX_RAGDOLL_PARTS];
	int					m_geomToJointIds[MAX_RAGDOLL_JOINT_IDS];

	int					m_farParents[MAX_RAGDOLL_JOINT_IDS];

	int					m_numBones{ 0 };
	int					m_numParts{ 0 };

	CEqStudioGeom*		m_studio{ nullptr };
};

