//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_RAGDOLL_PARTS 32

class CEqStudioGeom;
class IPhysicsObject;
class IPhysicsJoint;

struct PhysRagdollData
{
	void Init();

	// get bones transformation for rendering (NOTE: before this operation, reset input bone transform to identity)
	void			GetVisualBonesTransforms(Matrix4x4 *bones) const;

	void			GetBoundingBox(Vector3D& mins, Vector3D& maxs) const;
	Vector3D		GetPosition() const;

	// sets bone tranformations (useful for animated death, etc)
	// you can setup from here a global transform by multipling all matrices on model transform
	void			SetBoneTransform(Matrix4x4 *bones, const Matrix4x4& translation);

	// wakes ragdoll
	void			Wake();

	// finds far parent bone in ragdoll
	int				ComputeAndGetFarParentOf(int bone);

	void			RefreshRagdollVisuals();
	void			Translate(const Vector3D &move);

// members

	IPhysicsObject*	m_partObjs[MAX_RAGDOLL_PARTS];
	IPhysicsJoint*	m_physJoints[MAX_RAGDOLL_PARTS];

	int				m_bodyPartIds[MAX_RAGDOLL_PARTS];
	int				m_jointToGeomIds[MAX_RAGDOLL_PARTS];
	int				m_geomToJointIds[128];

	int				m_farParents[128];
	Matrix4x4		m_geomTransforms[128];

	int				m_numBones;
	int				m_numParts;

	CEqStudioGeom*	m_studio;
};

PhysRagdollData*	CreateRagdoll(CEqStudioGeom* pModel);
void		DestroyRagdoll(PhysRagdollData* ragdoll);
