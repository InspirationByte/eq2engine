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
	int					GetBoneIdx(int jointIdx) const;
	int					GetJointIdx(int boneIdx) const;

protected:
	// finds far parent bone in ragdoll
	int					CalcFarParent(int bone);

	static constexpr int MAX_RAGDOLL_PARTS = 32;

	ArrayCRef<StudioJoint>	m_studioBones{ nullptr };
	Array<IPhysicsObject*>	m_partObjs{ PP_SL };
	Array<IPhysicsJoint*>	m_physJoints{ PP_SL };

	Map<int16, int16>	m_jointToBoneIds{ PP_SL };
	Map<int16, int16>	m_boneToJointIds{ PP_SL };

	Map<int16, int16>	m_farParents{ PP_SL };
};

