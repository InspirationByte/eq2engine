//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics ragdoll on eqPhysics
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqPhysicsWorld;
class CEqRigidBody;
class CEqPointConstraint;
class CEqAngularLimitConstraint;
class CEqStudioGeom;
struct StudioJoint;

class CPhysicsRagdoll
{
public:
	CPhysicsRagdoll(CEqPhysicsWorld& physWorld, CEqStudioGeom* model);
	~CPhysicsRagdoll();

	// adds bodies and joints to the world (starts simulation)
	void				AddToWorld();
	void				RemoveFromWorld();
	bool				IsInWorld() const { return m_inWorld; }

	// get bones transformation for rendering, relative to GetPosition() (NOTE: before this operation, reset input bone transform to identity)
	void				GetVisualBonesTransforms(Matrix4x4* bones) const;

	BoundingBox			GetBoundingBox() const;
	Vector3D			GetPosition() const;

	// sets bone tranformations (useful for animated death, etc)
	// you can setup from here a global transform by multipling all matrices on model transform
	void				SetBoneTransform(const Matrix4x4* bones, const Matrix4x4& translation);

	void				Wake();
	void				Freeze();

	template<typename F>
	void				ForEachBodyPart(F func);

	void				Translate(const Vector3D& move);
	void				ResetVelocities();

	// bone transformation in body part space
	const Matrix4x4&	GetPartBoneFrame(int partIdx) const;
	int					GetBoneIdx(int partIdx) const;
	int					GetPartIdx(int boneIdx) const;

	CEqRigidBody*		GetPartBody(int partIdx) const;

protected:
	struct Part
	{
		CEqRigidBody*				body{ nullptr };
		CEqPointConstraint*			pointConstraint{ nullptr };
		CEqAngularLimitConstraint*	angularConstraint{ nullptr };
		Matrix4x4					boneFrame{ identity4 };
	};

	// finds far parent bone in ragdoll
	int					CalcFarParent(int bone);

	CEqPhysicsWorld&		m_physWorld;
	ArrayCRef<StudioJoint>	m_studioBones{ nullptr };
	Array<Part>				m_parts{ PP_SL };

	Map<int16, int16>		m_partToBoneIds{ PP_SL };
	Map<int16, int16>		m_boneToPartIds{ PP_SL };
	Map<int16, int16>		m_farParents{ PP_SL };

	bool					m_inWorld{ false };
};

template<typename F>
void CPhysicsRagdoll::ForEachBodyPart(F func)
{
	for (Part& part : m_parts)
	{
		if (part.body)
			func(part.body);
	}
}
