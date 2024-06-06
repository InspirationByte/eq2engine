//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ragdoll utilites, etc
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "ragdoll.h"
#include "studio/StudioGeom.h"

#include "render/IDebugOverlay.h"
#include "animating/BoneSetup.h"
#include "physics/PhysicsCollisionGroup.h"
#include "dkphysics/IDkPhysics.h"

#define RAGDOLL_LINEAR_LIMIT (0.25)

void PhysRagdollData::Init()
{
	m_numBones = 0;
	m_numParts = 0;

	memset(m_partObjs, 0, sizeof(m_partObjs));
	memset(m_physJoints, 0, sizeof(m_physJoints));
	memset(m_jointToGeomIds, -1, sizeof(m_jointToGeomIds));
	memset(m_geomToJointIds, -1, sizeof(m_geomToJointIds));
	memset(m_farParents, -1, sizeof(m_farParents));

	for (int i = 0; i < 128; i++)
	{
		m_geomTransforms[i] = identity4;
	}
}

// finds far parent bone in ragdoll
int PhysRagdollData::ComputeAndGetFarParentOf(int bone)
{
	const int parentBone = m_studio->GetJoints()[bone].parent;
	if(parentBone != -1)
	{
		if(m_geomToJointIds[parentBone] == -1)
		{
			// continue hierarchy
			return ComputeAndGetFarParentOf(parentBone);
		}
		else
		{
			// this is a needed parent with a ragdoll part.
			return parentBone;
		}
	}

	return -1;
}

#define COLLIDE_RAGDOLL (COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES)

PhysRagdollData* CreateRagdoll(CEqStudioGeom* pModel)
{
	if(!pModel)
		return nullptr;

	const StudioPhysData& physModel = pModel->GetPhysData();
	const studioHdr_t& studio = pModel->GetStudioHdr();
	ArrayCRef<StudioJoint> studioJoints = pModel->GetJoints();

	const int type = physModel.usageType;

	if(type == PHYSMODEL_USAGE_RAGDOLL)
	{
		PhysRagdollData* newRagdoll = PPNew PhysRagdollData;
		newRagdoll->Init();

		const int numPhysJoints = physModel.joints.numElem();
		const int numParts = physModel.objects.numElem();

		newRagdoll->m_numBones = numPhysJoints;
		newRagdoll->m_numParts = numParts;
		newRagdoll->m_studio = pModel;

		// build joint remap table
		for(int i = 0; i < numPhysJoints; i++)
		{
			for(int j = 0; j < studio.numBones; j++)
			{
				if(!CString::CompareCaseIns(studioJoints[j].bone->name, physModel.joints[i].name))
				{
					// assign index
					newRagdoll->m_jointToGeomIds[i] = j;
					newRagdoll->m_geomToJointIds[j] = i;

					continue;
				}
			}
		}

		// build far parental table
		for(int i = 0; i < studio.numBones; i++)
		{
			newRagdoll->m_farParents[i] = newRagdoll->ComputeAndGetFarParentOf(i);

			Matrix4x4 transform = identity4;

			const int real_parent = newRagdoll->m_farParents[i];
			if(real_parent != -1)
			{
				transform = studioJoints[i].absTrans * !studioJoints[real_parent].absTrans;
			}

			newRagdoll->m_geomTransforms[i] = transform;
		}

		// create objects of ragdoll
		for(int i = 0; i < numParts; i++)
		{
			newRagdoll->m_partObjs[i] = physics->CreateObject( &physModel, i);

			newRagdoll->m_partObjs[i]->SetContents( COLLISION_GROUP_DEBRIS );
			newRagdoll->m_partObjs[i]->SetCollisionMask( COLLIDE_RAGDOLL | COLLISION_GROUP_DEBRIS );

			newRagdoll->m_partObjs[i]->SetSleepTheresholds(20,20);
			newRagdoll->m_partObjs[i]->SetDamping(0.01f,0.05f);
			newRagdoll->m_partObjs[i]->SetActivationState(PS_ACTIVE);

			newRagdoll->m_bodyPartIds[i] = physModel.objects[i].desc.bodyPartId;

			newRagdoll->m_partObjs[i]->SetUserData(reinterpret_cast<void*>(newRagdoll->m_bodyPartIds[i]));
		}

		// create joints
		for(int i = 0; i < numPhysJoints; i++)
		{
			int object_partindexA = physModel.joints[i].objA;
			int object_partindexB = physModel.joints[i].objB;

			IPhysicsObject* partA = newRagdoll->m_partObjs[object_partindexA];
			IPhysicsObject*	partB = newRagdoll->m_partObjs[object_partindexB];

			// get a bone transformation
			const Matrix4x4& bone_transform = studioJoints[newRagdoll->m_jointToGeomIds[i]].absTrans;

			const Vector3D linkA_pos = bone_transform.rows[3].xyz() - newRagdoll->m_partObjs[object_partindexA]->GetPosition();
			const Vector3D linkB_pos = bone_transform.rows[3].xyz() - newRagdoll->m_partObjs[object_partindexB]->GetPosition();

			Matrix4x4 local_a = bone_transform;
			Matrix4x4 local_b = bone_transform;

			local_a.setTranslation( linkA_pos );
			local_b.setTranslation( linkB_pos );

			// create constraints
			newRagdoll->m_physJoints[i] = physics->CreateJoint(partA, partB, local_a, local_b, true);

			// set limits
			newRagdoll->m_physJoints[i]->SetAngularLowerLimit(physModel.joints[i].minLimit);
			newRagdoll->m_physJoints[i]->SetAngularUpperLimit(physModel.joints[i].maxLimit);

			newRagdoll->m_physJoints[i]->SetLinearLowerLimit( Vector3D(-RAGDOLL_LINEAR_LIMIT) );
			newRagdoll->m_physJoints[i]->SetLinearUpperLimit( Vector3D(RAGDOLL_LINEAR_LIMIT) );
		}

		return newRagdoll;
	}
	else
	{
		MsgError("Invalid physics model usage for %s (excepted PHYSMODEL_USAGE_RAGDOLL)\n", pModel->GetName());
	}

	return nullptr;
}

void PhysRagdollData::GetBoundingBox(Vector3D &mins, Vector3D &maxs) const
{
	BoundingBox aabb;

	for(int i = 0; i < m_numParts; i++)
	{
		if (!m_partObjs[i])
			continue;
		Vector3D partAABBMins;
		Vector3D partAABBMaxs;

		m_partObjs[i]->GetBoundingBox(partAABBMins, partAABBMaxs);

		aabb.AddVertex(partAABBMins);
		aabb.AddVertex(partAABBMaxs);
	}

	mins = aabb.minPoint;
	maxs = aabb.maxPoint;
}

Vector3D PhysRagdollData::GetPosition() const
{
	Vector3D ragdoll_bboxmin;
	Vector3D ragdoll_bboxmax;

	GetBoundingBox(ragdoll_bboxmin, ragdoll_bboxmax);

	return (ragdoll_bboxmin+ragdoll_bboxmax)*0.5f;
}

void PhysRagdollData::GetVisualBonesTransforms(Matrix4x4 *bones) const
{
	Matrix4x4 offsetTranslate = identity4;
	offsetTranslate.setTranslation(-GetPosition());

	const studioHdr_t& studio = m_studio->GetStudioHdr();

	for(int i = 0; i < studio.numBones; i++)
	{
		const int ragdoll_joint_index = m_geomToJointIds[i];

		if(ragdoll_joint_index != -1)
		{
			Matrix4x4 bone_global = m_physJoints[ragdoll_joint_index]->GetGlobalTransformA();
			bones[i] = bone_global*offsetTranslate;
		}
		else
		{
			const int far_parent = m_farParents[i];

			if(far_parent != -1)
			{
				Matrix4x4 bone_transform = m_geomTransforms[i];

				bones[i] = bone_transform * bones[far_parent];
			}
		}
	}
}

// sets bone tranformations (useful for animated death, etc)
// you can setup from here a global transform
void PhysRagdollData::SetBoneTransform(Matrix4x4 *bones, const Matrix4x4& translation)
{
	// set part transform
	for(int i = 0; i < m_numParts; i++)
	{
		if(m_partObjs[i])
			m_partObjs[i]->SetTransformFromMatrix( (!m_physJoints[i]->GetFrameTransformA() * bones[m_jointToGeomIds[i]])*translation);
	}

	RefreshRagdollVisuals();
}

void PhysRagdollData::Translate(const Vector3D &move)
{
	for(int i = 0; i < m_numParts; i++)
	{
		if(m_partObjs[i])
			m_partObjs[i]->SetPosition(m_partObjs[i]->GetPosition() + move);
	}

	RefreshRagdollVisuals();
}

void PhysRagdollData::RefreshRagdollVisuals()
{
	// refresh joint transform
	for(int i = 0; i < m_numBones; i++)
	{
		if(m_physJoints[i])
		{
			m_physJoints[i]->UpdateTransform();
		}
	}
}

// wakes ragdoll
void PhysRagdollData::Wake()
{
	// set part transform
	for(int i = 0; i < m_numParts; i++)
	{
		if(m_partObjs[i])
			m_partObjs[i]->WakeUp();
	}

	RefreshRagdollVisuals();
}

void DestroyRagdoll(PhysRagdollData* ragdoll)
{
	if(!ragdoll)
		return;

	// destroy all bones and objects
	for(int i = 0; i < ragdoll->m_numBones; i++)
	{
		if(ragdoll->m_physJoints[i])
		{
			physics->DestroyPhysicsJoint(ragdoll->m_physJoints[i]);
		}
	}

	for(int i = 0; i < ragdoll->m_numParts; i++)
	{
		if(ragdoll->m_partObjs[i])
		{
			physics->DestroyPhysicsObject(ragdoll->m_partObjs[i]);
		}
	}

	delete ragdoll;
}