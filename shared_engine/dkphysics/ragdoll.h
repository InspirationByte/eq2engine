//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define MAX_RAGDOLL_PARTS 32

class CEqStudioGeom;
class IPhysicsObject;
class IPhysicsJoint;

struct ragdoll_t
{
	void Init()
	{
		m_numBones = 0;
		m_numParts = 0;

		memset(m_pParts,0,sizeof(m_pParts));
		memset(m_pJoints,0,sizeof(m_pJoints));
		memset(m_pBoneToVisualIndices,-1,sizeof(m_pBoneToVisualIndices));
		memset(m_pBoneToRagdollIndices,-1,sizeof(m_pBoneToRagdollIndices));
		memset(m_pBoneMerge_far_parents,-1,sizeof(m_pBoneMerge_far_parents));

		for(int i = 0; i < 128; i++)
		{
			m_pBoneMerge_Transformations[i] = identity4();
		}
	}

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

	IPhysicsObject*	m_pParts[MAX_RAGDOLL_PARTS];
	IPhysicsJoint*	m_pJoints[MAX_RAGDOLL_PARTS];

	int				m_nBodyParts[MAX_RAGDOLL_PARTS];
	int				m_pBoneToVisualIndices[MAX_RAGDOLL_PARTS];
	int				m_pBoneToRagdollIndices[128];

	int				m_pBoneMerge_far_parents[128];
	Matrix4x4		m_pBoneMerge_Transformations[128];

	int				m_numBones;
	int				m_numParts;

	CEqStudioGeom*	m_pReferenceModel;
};

ragdoll_t*	CreateRagdoll(CEqStudioGeom* pModel);
void		DestroyRagdoll(ragdoll_t* ragdoll);
