//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "ragdoll.h"
#include "studio/StudioGeom.h"

#include "animating/BoneSetup.h"
#include "dkphysics/IDkPhysics.h"

static constexpr float RAGDOLL_DEFAULT_LINEAR_LIMIT = 0.0025;

CPhysRagdollData::~CPhysRagdollData()
{
	// destroy all bones and objects
	for (int i = 0; i < m_physJoints.numElem(); i++)
	{
		if (m_physJoints[i])
			physics->DestroyPhysicsJoint(m_physJoints[i]);
	}

	for (int i = 0; i < m_partObjs.numElem(); i++)
	{
		if (m_partObjs[i])
			physics->DestroyPhysicsObject(m_partObjs[i]);
	}
}

CPhysRagdollData::CPhysRagdollData(CEqStudioGeom* pModel)
{
	ASSERT(pModel);

	const StudioPhysData& physModel = pModel->GetPhysData();
	if (physModel.usageType != PHYSMODEL_USAGE_RAGDOLL)
	{
		ASSERT_FAIL("Invalid physics model usage for %s (excepted PHYSMODEL_USAGE_RAGDOLL)", pModel->GetName());
		return;
	}

	const studioHdr_t& studio = pModel->GetStudioHdr();
	m_studioBones = pModel->GetJoints();

	const int numPhysJoints = physModel.joints.numElem();
	const int numParts = physModel.objects.numElem();

	// build joint remap table
	for (int jointIdx = 0; jointIdx < numPhysJoints; jointIdx++)
	{
		for (int boneIdx = 0; boneIdx < studio.numBones; boneIdx++)
		{
			if (!CString::CompareCaseIns(m_studioBones[boneIdx].bone->name, physModel.joints[jointIdx].name))
			{
				// assign index
				m_jointToBoneIds.insert(jointIdx, boneIdx);
				m_boneToJointIds.insert(boneIdx, jointIdx);
				continue;
			}
		}
	}

	// build far parental table
	for (int boneIdx = 0; boneIdx < studio.numBones; boneIdx++)
	{
		const int parentIdx = CalcFarParent(boneIdx);
		if(parentIdx != -1)
			m_farParents.insert(boneIdx, parentIdx);
	}

	// create objects of ragdoll
	m_partObjs.setNum(numParts);
	for (int i = 0; i < numParts; i++)
	{
		IPhysicsObject* physObj = physics->CreateStudioObject(physModel, i);
		const int bodyPartId = physModel.objects[i].desc.bodyPartId;
		physObj->SetUserData(reinterpret_cast<void*>(bodyPartId));
		m_partObjs[i] = physObj;
	}

	// create joints
	m_physJoints.setNum(numPhysJoints);
	for (int i = 0; i < numPhysJoints; i++)
	{
		const int boneIdx = GetBoneIdx(i);
		if (boneIdx == -1)
			continue;

		// get a bone transformation
		const Matrix4x4& boneAbsTrs = m_studioBones[boneIdx].absTrans;

		const int objIdxA = physModel.joints[i].objA;
		const int objIdxB = physModel.joints[i].objB;
		const Vector3D linkPosA = boneAbsTrs.rows[3].xyz() - physModel.objects[objIdxA].desc.offset;
		const Vector3D linkPosB = boneAbsTrs.rows[3].xyz() - physModel.objects[objIdxB].desc.offset;

		Matrix4x4 localTrsA = boneAbsTrs;
		Matrix4x4 localTrsB = boneAbsTrs;
		localTrsA.setTranslation(linkPosA);
		localTrsB.setTranslation(linkPosB);

		// create constraints
		IPhysicsJoint* physJoint = physics->CreateJoint(m_partObjs[objIdxA], m_partObjs[objIdxB], localTrsA, localTrsB, true);
		physJoint->SetAngularLowerLimit(physModel.joints[i].minLimit);
		physJoint->SetAngularUpperLimit(physModel.joints[i].maxLimit);
		physJoint->SetLinearLowerLimit(Vector3D(-RAGDOLL_DEFAULT_LINEAR_LIMIT));
		physJoint->SetLinearUpperLimit(Vector3D(RAGDOLL_DEFAULT_LINEAR_LIMIT));

		m_physJoints[i] = physJoint;
	}
}

// finds far parent bone in ragdoll
int CPhysRagdollData::CalcFarParent(int bone)
{
	const int parentBone = m_studioBones[bone].parent;
	if (parentBone == -1)
		return -1;

	if (GetJointIdx(parentBone) == -1)
	{
		// continue hierarchy
		return CalcFarParent(parentBone);
	}

	// this is a needed parent with a ragdoll part.
	return parentBone;
}

BoundingBox CPhysRagdollData::GetBoundingBox() const
{
	BoundingBox bbox;
	for (int i = 0; i < m_partObjs.numElem(); i++)
	{
		if (!m_partObjs[i])
			continue;

		Vector3D partMins;
		Vector3D partMaxs;
		m_partObjs[i]->GetBoundingBox(partMins, partMaxs);

		bbox.AddVertex(partMins);
		bbox.AddVertex(partMaxs);
	}
	return bbox;
}

Vector3D CPhysRagdollData::GetPosition() const
{
	return GetBoundingBox().GetCenter();
}

void CPhysRagdollData::GetVisualBonesTransforms(Matrix4x4* bones) const
{
	Matrix4x4 offsetTranslate = identity4;
	offsetTranslate.setTranslation(-GetPosition());

	for (int boneIdx = 0; boneIdx < m_studioBones.numElem(); boneIdx++)
	{
		const int jointIdx = GetJointIdx(boneIdx);
		if (jointIdx != -1)
		{
			Matrix4x4 boneGlobalTrs = m_physJoints[jointIdx]->GetGlobalTransformA();
			bones[boneIdx] = boneGlobalTrs * offsetTranslate;
		}
		else
		{
			const auto it = m_farParents.find(boneIdx);
			if (!it.atEnd())
				bones[boneIdx] = (m_studioBones[boneIdx].absTrans * m_studioBones[*it].invAbsTrans) * bones[*it];
		}
	}
}

// sets bone tranformations (useful for animated death, etc)
// you can setup from here a global transform
void CPhysRagdollData::SetBoneTransform(Matrix4x4* bones, const Matrix4x4& translation)
{
	// set part transform
	for (int i = 0; i < m_partObjs.numElem(); i++)
	{
		if (m_partObjs[i])
		{
			const int boneIdx = GetBoneIdx(i);
			m_partObjs[i]->SetTransformFromMatrix((!m_physJoints[i]->GetFrameTransformA() * bones[boneIdx]) * translation);
		}
	}

	RefreshRagdollVisuals();
}

void CPhysRagdollData::Translate(const Vector3D& move)
{
	ForEachBodyPart([&](IPhysicsObject* obj) {
		obj->SetPosition(obj->GetPosition() + move);
	});
	RefreshRagdollVisuals();
}

void CPhysRagdollData::RefreshRagdollVisuals()
{
	ForEachJoint([](IPhysicsJoint* joint) {
		joint->UpdateTransform();
	});
}

// wakes ragdoll
void CPhysRagdollData::Wake()
{
	ForEachBodyPart([](IPhysicsObject* obj) {
		obj->WakeUp();
	});

	RefreshRagdollVisuals();
}

void CPhysRagdollData::Freeze()
{
	for (int i = 0; i < m_partObjs.numElem(); i++)
	{
		if (m_partObjs[i])
			m_partObjs[i]->SetActivationState(PS_FROZEN);
	}

}

void CPhysRagdollData::ResetVelocities()
{
	ForEachBodyPart([](IPhysicsObject* obj) {
		obj->SetVelocity(vec3_zero);
		obj->SetAngularVelocity(vec3_unit, 0.0);
	});
}

Matrix4x4 CPhysRagdollData::GetJointTransformA(int idx) const
{
	return m_physJoints[idx]->GetFrameTransformA();
}

int CPhysRagdollData::GetBoneIdx(int jointIdx) const
{
	const auto it = m_jointToBoneIds.find(jointIdx);
	return it.atEnd() ? -1 : *it;
}

int CPhysRagdollData::GetJointIdx(int boneIdx) const
{
	const auto it = m_boneToJointIds.find(boneIdx);
	return it.atEnd() ? -1 : *it;
}